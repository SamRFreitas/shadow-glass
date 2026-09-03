import CLibDataChannel
import Darwin
import Foundation

// Piece 13: the real LowLatencyTransport implementation, replacing
// FakeTransport (deleted). This is piece 12's SignalingClientTest
// (deleted too), reshaped to fit the same connect/send/callback contract
// the UI already talks to through Transport.swift — so ContentView barely
// changes at all to start using a real connection instead of the fake one.
//
// Networking is plain BSD sockets (not Network.framework) and negotiation
// is automatic WebRTC signaling (docs/protocol.md) against Windows's
// signaling_test.cpp — see docs/mac-vs-windows-networking.html for the
// full story of why, and docs/LEARNING_LOG.md for the real bug that took
// a whole session to find (nothing kept the old test's instance alive).
// Here, that problem doesn't come back on its own: ContentView's own
// `transport` property is what keeps this object alive for as long as
// the app runs, the same way it already did for FakeTransport.
final class LibDataChannelTransport: LowLatencyTransport {
    var onStatusChanged: ((ConnectionStatus) -> Void)?
    var onMessageReceived: ((String) -> Void)?

    private var pc: Int32 = -1
    private var dc: Int32 = -1
    private var socketFD: Int32 = -1
    private var receiveBuffer = ""

    // Fixed by docs/protocol.md — not configurable from the UI yet.
    private let port: UInt16 = 45180

    // Serializes writes to socketFD: libdatachannel's own callbacks
    // (offer/candidate generation) fire from its internal threads, not
    // ours, so more than one could try to send() at the same instant
    // without this.
    private let sendQueue = DispatchQueue(label: "shadow-glass.signaling-send")

    // Mirrors docs/protocol.md exactly: only 'type' is always present,
    // the rest depend on which kind of message it is.
    private struct SignalingMessage: Codable {
        let type: String
        var sdp: String?
        var candidate: String?
        var mid: String?
    }

    private func log(_ message: String) {
        // fflush matters here for the same reason it did in piece 12's
        // test: stdout isn't reliably flushed on its own once redirected
        // away from a real terminal.
        print(message)
        fflush(stdout)
    }

    // onStatusChanged/onMessageReceived feed SwiftUI @State in
    // ContentView, which must only be touched from the main thread —
    // every call site below goes through these two instead of calling
    // the closures directly, since all of them fire from background
    // threads (our own socket thread, or libdatachannel's internal ones).
    private func notifyStatus(_ status: ConnectionStatus) {
        DispatchQueue.main.async { [weak self] in self?.onStatusChanged?(status) }
    }
    private func notifyMessage(_ message: String) {
        DispatchQueue.main.async { [weak self] in self?.onMessageReceived?(message) }
    }

    func connect(to host: String) {
        setvbuf(stdout, nil, _IOLBF, 0)
        rtcInitLogger(RTC_LOG_WARNING, nil)
        notifyStatus(.connecting)

        // connect() and recv() below are blocking BSD calls — running
        // them on the main thread would freeze the whole UI.
        let port = self.port
        DispatchQueue.global(qos: .userInitiated).async { [weak self] in
            self?.connectAndRun(host: host, port: port)
        }
    }

    private func connectAndRun(host: String, port: UInt16) {
        let fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)
        guard fd >= 0 else {
            log("transport: socket() failed: \(String(cString: strerror(errno)))")
            notifyStatus(.disconnected)
            return
        }

        var addr = sockaddr_in()
        addr.sin_family = sa_family_t(AF_INET)
        addr.sin_port = port.bigEndian // network byte order — same idea as Winsock's htons()
        guard inet_pton(AF_INET, host, &addr.sin_addr) == 1 else {
            log("transport: invalid host '\(host)'")
            close(fd)
            notifyStatus(.disconnected)
            return
        }

        let connectResult = withUnsafePointer(to: &addr) { addrPtr -> Int32 in
            addrPtr.withMemoryRebound(to: sockaddr.self, capacity: 1) { sockaddrPtr in
                Darwin.connect(fd, sockaddrPtr, socklen_t(MemoryLayout<sockaddr_in>.size))
            }
        }
        guard connectResult == 0 else {
            log("transport: connect() failed: \(String(cString: strerror(errno))) — is signaling_test.exe running?")
            close(fd)
            notifyStatus(.disconnected)
            return
        }

        socketFD = fd
        log("transport: connected to \(host):\(port)")
        startPeerConnection()
        receiveLoop(fd: fd)
    }

    private func startPeerConnection() {
        var config = rtcConfiguration()
        pc = rtcCreatePeerConnection(&config)
        guard pc >= 0 else {
            log("transport: failed to create peer connection (\(pc))")
            notifyStatus(.disconnected)
            return
        }

        // A C function pointer can't capture `self` the way a closure
        // would, so the library hands this opaque pointer back to every
        // callback instead.
        let context = Unmanaged.passUnretained(self).toOpaque()
        rtcSetUserPointer(pc, context)

        rtcSetLocalDescriptionCallback(pc) { _, sdp, type, ptr in
            guard let ptr, let sdp, let type else { return }
            let instance = Unmanaged<LibDataChannelTransport>.fromOpaque(ptr).takeUnretainedValue()
            instance.sendSignalingMessage(SignalingMessage(type: String(cString: type), sdp: String(cString: sdp)))
        }
        rtcSetLocalCandidateCallback(pc) { _, cand, mid, ptr in
            guard let ptr, let cand, let mid else { return }
            let instance = Unmanaged<LibDataChannelTransport>.fromOpaque(ptr).takeUnretainedValue()
            instance.sendSignalingMessage(SignalingMessage(type: "candidate", candidate: String(cString: cand), mid: String(cString: mid)))
        }

        // We're always the offering side (docs/protocol.md) — creating a
        // data channel is what kicks off negotiation.
        dc = rtcCreateDataChannel(pc, "shadow-glass")
        guard dc >= 0 else {
            log("transport: failed to create data channel (\(dc))")
            notifyStatus(.disconnected)
            return
        }
        rtcSetUserPointer(dc, context)
        rtcSetOpenCallback(dc) { _, ptr in
            guard let ptr else { return }
            let instance = Unmanaged<LibDataChannelTransport>.fromOpaque(ptr).takeUnretainedValue()
            instance.log("transport: DataChannel is open!")
            instance.notifyStatus(.connected)
        }
        rtcSetMessageCallback(dc) { _, message, size, ptr in
            guard let ptr, let message else { return }
            let instance = Unmanaged<LibDataChannelTransport>.fromOpaque(ptr).takeUnretainedValue()
            let text = String(data: Data(bytes: message, count: Int(size)), encoding: .utf8) ?? "<non-utf8 message>"
            instance.notifyMessage(text)
        }
    }

    // --- LowLatencyTransport.send ---

    func send(_ message: String) {
        guard dc >= 0 else {
            log("transport: send() called before the DataChannel is open, ignoring")
            return
        }
        message.withCString { cString in
            _ = rtcSendMessage(dc, cString, -1) // negative size = send as a text message
        }
    }

    // --- Sending our side's signaling messages to Windows ---

    private func sendSignalingMessage(_ message: SignalingMessage) {
        guard let json = try? JSONEncoder().encode(message) else { return }
        var line = json
        line.append(0x0A) // '\n' — same newline-delimited framing as the Windows side
        sendQueue.async { [weak self] in
            guard let self, self.socketFD >= 0 else { return }
            let fd = self.socketFD
            line.withUnsafeBytes { buffer in
                _ = Darwin.send(fd, buffer.baseAddress, buffer.count, 0)
            }
        }
    }

    // --- Reading Windows's signaling messages back ---

    // Same reasoning as signaling_test.cpp's recv loop: TCP only
    // guarantees a stream of bytes, not message boundaries, so we
    // accumulate into a string and pull out each complete line. recv()
    // blocks, so this runs on its own background thread (started from
    // connectAndRun) for as long as the connection stays open.
    private func receiveLoop(fd: Int32) {
        var buffer = [UInt8](repeating: 0, count: 65536)
        while true {
            let bytesRead = buffer.withUnsafeMutableBytes { recv(fd, $0.baseAddress, $0.count, 0) }
            if bytesRead <= 0 {
                if bytesRead < 0 {
                    log("transport: recv() error: \(String(cString: strerror(errno)))")
                } else {
                    log("transport: Windows closed the signaling connection")
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
            log("transport: could not parse line as JSON: \(line)")
            return
        }

        switch message.type {
        case "answer":
            guard let sdp = message.sdp else { return }
            rtcSetRemoteDescription(pc, sdp, "answer")
        case "candidate":
            guard let candidate = message.candidate, let mid = message.mid else { return }
            rtcAddRemoteCandidate(pc, candidate, mid)
        default:
            log("transport: unrecognized message type '\(message.type)'")
        }
    }
}
