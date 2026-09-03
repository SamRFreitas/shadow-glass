import CLibDataChannel
import Darwin
import Foundation

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
//
// Networking here is plain BSD sockets (socket/connect/send/recv), not
// Apple's higher-level Network framework — that was the original
// approach, and it silently never worked: NWConnection's own state
// callback never fired even once, on this Mac, in any of several launch
// methods (swift run, Xcode, a real .app bundle). The likely cause and
// the full comparison with how Windows does the exact same thing is
// documented visually in docs/mac-vs-windows-networking.html; the short
// version is that macOS's "Local Network" privacy permission only gates
// Network.framework/Bonjour-style APIs, and plain POSIX sockets were
// never subject to it — which our own `nc` tests (pieces 10-11) already
// proved worked instantly, with no popup, all along. This is also
// conceptually the exact same API signaling_test.cpp already uses on
// Windows via Winsock2 (itself modeled on BSD sockets) — the two sides
// now mirror each other closely, not just in wire format but in API
// shape.
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
    private var socketFD: Int32 = -1
    private var receiveBuffer = ""

    // Serializes writes to socketFD: libdatachannel's own callbacks
    // (offer/candidate generation) fire from its internal threads, not
    // ours, so more than one could try to send() at the same instant
    // without this.
    private let sendQueue = DispatchQueue(label: "shadow-glass.signaling-send")

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

        // connect() and recv() below are blocking BSD calls — running them
        // on the main thread would freeze the whole UI, so the entire
        // networking side runs on a background thread instead.
        DispatchQueue.global(qos: .userInitiated).async { [weak self] in
            self?.connectAndRun(host: host, port: port)
        }
    }

    private func connectAndRun(host: String, port: UInt16) {
        let fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)
        guard fd >= 0 else {
            print("signaling client: socket() failed: \(String(cString: strerror(errno)))")
            return
        }

        var addr = sockaddr_in()
        addr.sin_family = sa_family_t(AF_INET)
        addr.sin_port = port.bigEndian // network byte order — same idea as Winsock's htons()
        guard inet_pton(AF_INET, host, &addr.sin_addr) == 1 else {
            print("signaling client: invalid host '\(host)'")
            close(fd)
            return
        }

        let connectResult = withUnsafePointer(to: &addr) { addrPtr -> Int32 in
            addrPtr.withMemoryRebound(to: sockaddr.self, capacity: 1) { sockaddrPtr in
                connect(fd, sockaddrPtr, socklen_t(MemoryLayout<sockaddr_in>.size))
            }
        }
        guard connectResult == 0 else {
            print("signaling client: connect() failed: \(String(cString: strerror(errno))) — is signaling_test.exe running?")
            close(fd)
            return
        }

        socketFD = fd
        print("signaling client: connected to \(host):\(port)")
        startPeerConnection()
        receiveLoop(fd: fd)
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
        sendQueue.async { [weak self] in
            guard let self, self.socketFD >= 0 else { return }
            let fd = self.socketFD
            line.withUnsafeBytes { buffer in
                _ = send(fd, buffer.baseAddress, buffer.count, 0)
            }
        }
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
    // accumulate into a string and pull out each complete line. recv()
    // blocks until data arrives, so this runs on its own background
    // thread (started from connectAndRun) for as long as the connection
    // stays open — there's no completion-handler style here, unlike
    // Network.framework, which is exactly the trade-off of using the
    // lower-level API directly.
    private func receiveLoop(fd: Int32) {
        var buffer = [UInt8](repeating: 0, count: 65536)
        while true {
            let bytesRead = buffer.withUnsafeMutableBytes { recv(fd, $0.baseAddress, $0.count, 0) }
            if bytesRead <= 0 {
                if bytesRead < 0 {
                    print("signaling client: recv() error: \(String(cString: strerror(errno)))")
                } else {
                    print("signaling client: Windows closed the signaling connection")
                }
                return
            }
            receiveBuffer += String(decoding: buffer[0..<bytesRead], as: UTF8.self)
            processBufferedLines()
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
