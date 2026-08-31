import Foundation

// The three states our UI cares about. Kept deliberately small — this is
// not meant to mirror every internal WebRTC connection state, just what
// the status label needs to show.
enum ConnectionStatus {
    case disconnected
    case connecting
    case connected
}

// Our own contract for "a low-latency connection to the other machine",
// independent of which library implements it underneath. The app (the UI
// in ContentView) only ever talks to this protocol, never directly to
// libdatachannel (or, later, libwebrtc) — so swapping the concrete
// implementation means writing a new type that conforms to this same
// protocol, with no changes anywhere else.
protocol LowLatencyTransport: AnyObject {
    var onStatusChanged: ((ConnectionStatus) -> Void)? { get set }
    var onMessageReceived: ((String) -> Void)? { get set }

    func connect(to host: String)
    func send(_ message: String)
}
