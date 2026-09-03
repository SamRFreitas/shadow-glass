import CLibDataChannel
import Darwin
import Foundation
import Network

// Piece 12: the Mac side finally does what a human did by hand in piece
// 8 (manually copy-pasting SDP/candidates between two consoles) — but
// automatically, over the exact wire format docs/protocol.md defines and
// pieces 9-11 already proved the Windows side speaks correctly:
//   1. Open a plain TCP connection to the Windows signaling port.
//   2. Create a PeerConnection + DataChannel (we're always the offering
//      side — docs/protocol.md's exchange has the Mac send first).
//   3. Every time the library generates our offer or one of our
//      candidates, send it to Windows as one JSON line.
//   4. Read whatever comes back, line by line, and feed the 'answer' and
//      'candidate' messages straight into the same PeerConnection.
//   5. If everything above is correct, the DataChannel opens for real —
//      the same milestone piece 8 proved by hand, now hands-free.
//
// Supersedes and replaces LibDataChannelOfferTest: everything that test
// did (create a PeerConnection, create a DataChannel, generate an offer)
// still happens here, plus the real signaling exchange around it.
final class SignalingClientTest {
    static func run(host: String, port: UInt16 = 45180) {
        // Kept alive on purpose, same reasoning as LibDataChannelOfferTest:
        // this object owns the connection's lifetime for as long as the
        // app runs. Still a throwaway test, not the real transport.
        let test = SignalingClientTest()
        test.start(host: host, port: port)
    }

    private var pc: Int32 = -1
    private var dc: Int32 = -1
    private var connection: NWConnection?
    private var receiveBuffer = ""

    // Mirrors docs/protocol.md exactly: only 'type' is always present,
    // the rest depend on which kind of message it is. Swift's compiler-
    // generated Codable conformance omits a nil Optional entirely when
    // encoding (not as a JSON null) — matching nlohmann::json's
    // .value(key, default) on the Windows side, which only looks at keys
    // that are actually there.
    private struct SignalingMessage: Codable {
        let type: String
        var sdp: String?
        var candidate: String?
        var mid: String?
    }

    private func start(host: String, port: UInt16) {
        // Same buffering bug as piece 5 — see LibDataChannelOfferTest's
        // history in docs/LEARNING_LOG.md if this ever needs re-explaining.
        setvbuf(stdout, nil, _IOLBF, 0)
        rtcInitLogger(RTC_LOG_WARNING, nil)

        // Connect the signaling channel first, and only start the
        // PeerConnection once it's actually open — that way there's never
        // a moment where the library has already generated an offer/
        // candidate with nowhere to send it yet.
        guard let nwPort = NWEndpoint.Port(rawValue: port) else {
            print("signaling client: invalid port \(port)")
            return
        }
        let connection = NWConnection(host: NWEndpoint.Host(host), port: nwPort, using: .tcp)
        self.connection = connection

        connection.stateUpdateHandler = { [weak self] state in
            guard let self else { return }
            switch state {
            case .ready:
                print("signaling client: connected to \(host):\(port)")
                self.receiveLine()
                self.startPeerConnection()
            case .failed(let error):
                print("signaling client: connection failed: \(error)")
            case .waiting(let error):
                print("signaling client: waiting to connect (\(error)) — is signaling_test.exe running?")
            default:
                break
            }
        }
        connection.start(queue: .main)
    }

    private func startPeerConnection() {
        var config = rtcConfiguration()
        pc = rtcCreatePeerConnection(&config)
        guard pc >= 0 else {
            print("signaling client: failed to create peer connection (\(pc))")
            return
        }

        // Same Unmanaged/rtcSetUserPointer bridging LibDataChannelOfferTest
        // used — a C function pointer can't capture `self` the way a
        // closure would, so the library hands this opaque pointer back to
        // every callback instead.
        let context = Unmanaged.passUnretained(self).toOpaque()
        rtcSetUserPointer(pc, context)

        rtcSetLocalDescriptionCallback(pc) { _, sdp, type, ptr in
            guard let ptr, let sdp, let type else { return }
            let instance = Unmanaged<SignalingClientTest>.fromOpaque(ptr).takeUnretainedValue()
            instance.sendSignalingMessage(SignalingMessage(type: String(cString: type), sdp: String(cString: sdp)))
        }
        rtcSetLocalCandidateCallback(pc) { _, cand, mid, ptr in
            guard let ptr, let cand, let mid else { return }
            let instance = Unmanaged<SignalingClientTest>.fromOpaque(ptr).takeUnretainedValue()
            instance.sendSignalingMessage(SignalingMessage(type: "candidate", candidate: String(cString: cand), mid: String(cString: mid)))
        }

        // We're the offering side — creating a data channel is what kicks
        // off negotiation (same reasoning as LibDataChannelOfferTest).
        dc = rtcCreateDataChannel(pc, "shadow-glass")
        guard dc >= 0 else {
            print("signaling client: failed to create data channel (\(dc))")
            return
        }
        rtcSetUserPointer(dc, context)
        rtcSetOpenCallback(dc) { _, ptr in
            guard let ptr else { return }
            let instance = Unmanaged<SignalingClientTest>.fromOpaque(ptr).takeUnretainedValue()
            instance.handleDataChannelOpen()
        }
        rtcSetMessageCallback(dc) { _, message, size, _ in
            guard let message else { return }
            let text = String(data: Data(bytes: message, count: Int(size)), encoding: .utf8) ?? "<non-utf8 message>"
            print("signaling client: message from Windows: \(text)")
        }
    }

    // --- Sending our side's messages to Windows ---

    private func sendSignalingMessage(_ message: SignalingMessage) {
        guard let json = try? JSONEncoder().encode(message) else { return }
        var line = json
        line.append(0x0A) // '\n' — same newline-delimited framing as the Windows side
        print("signaling client: sending \(message.type)")
        connection?.send(content: line, completion: .contentProcessed { error in
            if let error {
                print("signaling client: send error: \(error)")
            }
        })
    }

    private func handleDataChannelOpen() {
        print("signaling client: DataChannel is open! (the real milestone — piece 8's manual test, automated)")
        "Hello from the Mac, automatically!".withCString { cString in
            _ = rtcSendMessage(dc, cString, -1) // negative size = send as a text message
        }
    }

    // --- Reading Windows's messages back ---

    // Same reasoning as signaling_test.cpp's recv loop: TCP only
    // guarantees a stream of bytes, not message boundaries, so we
    // accumulate into a string and pull out each complete line.
    private func receiveLine() {
        connection?.receive(minimumIncompleteLength: 1, maximumLength: 65536) { [weak self] data, _, isComplete, error in
            guard let self else { return }
            if let data, let text = String(data: data, encoding: .utf8) {
                self.receiveBuffer += text
                self.processBufferedLines()
            }
            if let error {
                print("signaling client: receive error: \(error)")
                return
            }
            if isComplete {
                print("signaling client: Windows closed the signaling connection")
                return
            }
            self.receiveLine()
        }
    }

    private func processBufferedLines() {
        while let newlineRange = receiveBuffer.range(of: "\n") {
            let line = String(receiveBuffer[..<newlineRange.lowerBound])
            receiveBuffer.removeSubrange(..<newlineRange.upperBound)
            handleLine(line)
        }
    }

    private func handleLine(_ line: String) {
        guard !line.isEmpty,
              let data = line.data(using: .utf8),
              let message = try? JSONDecoder().decode(SignalingMessage.self, from: data) else {
            print("signaling client: could not parse line as JSON: \(line)")
            return
        }

        switch message.type {
        case "answer":
            guard let sdp = message.sdp else { return }
            print("signaling client: received 'answer' (\(sdp.utf8.count) bytes of SDP) — handing it to the PeerConnection")
            rtcSetRemoteDescription(pc, sdp, "answer")
        case "candidate":
            guard let candidate = message.candidate, let mid = message.mid else { return }
            print("signaling client: received a candidate: \(candidate) (mid=\(mid)) — adding it to the PeerConnection")
            rtcAddRemoteCandidate(pc, candidate, mid)
        default:
            print("signaling client: unrecognized message type '\(message.type)'")
        }
    }
}
