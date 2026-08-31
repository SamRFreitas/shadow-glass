import Foundation

// A fake implementation of LowLatencyTransport with no real networking at
// all — it just simulates a connection and echoes back whatever is sent,
// after short delays (so the UI has something to actually observe
// changing, instead of updating instantly). Its only purpose is to prove
// the protocol's shape works end-to-end with the UI (status label, button,
// received-message display) before the real libdatachannel-backed
// implementation exists. Once that real implementation lands, this file
// gets deleted — nothing else should depend on it directly.
final class FakeTransport: LowLatencyTransport {
    var onStatusChanged: ((ConnectionStatus) -> Void)?
    var onMessageReceived: ((String) -> Void)?

    func connect(to host: String) {
        onStatusChanged?(.connecting)
        DispatchQueue.main.asyncAfter(deadline: .now() + 1) { [weak self] in
            self?.onStatusChanged?(.connected)
        }
    }

    func send(_ message: String) {
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) { [weak self] in
            self?.onMessageReceived?("(fake echo) \(message)")
        }
    }
}
