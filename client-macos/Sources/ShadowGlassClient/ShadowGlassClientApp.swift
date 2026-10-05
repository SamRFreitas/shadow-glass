import SwiftUI

// Entry point of the app. `@main` tells Swift this struct is where the
// program starts running — it replaces a traditional `main()` function
// when using SwiftUI's `App` protocol. Note: this can't live in a file
// named `main.swift` (that filename has its own implicit-entry-point
// rule that conflicts with an explicit `@main`), which is why the old
// `main.swift` placeholder is being removed in this same step.
@main
struct ShadowGlassClientApp: App {
    var body: some Scene {
        WindowGroup {
            ContentView()
        }
    }
}

// The main (and only, for now) screen. Wired to a LowLatencyTransport —
// now LibDataChannelTransport (piece 13), the real connection, through
// the protocol only, exactly as planned back when FakeTransport
// (deleted) was still the only implementation: this view never needed
// to change when the real one replaced the fake one.
struct ContentView: View {
    @State private var status: ConnectionStatus = .disconnected
    @State private var lastReceivedMessage = ""

    // `let` because swapping the transport implementation is a one-line
    // change here, not something that touches anything below. This
    // property is also what keeps the transport object alive for as long
    // as the app runs — see LibDataChannelTransport.swift's header
    // comment for why that specifically matters.
    private let transport: LowLatencyTransport = LibDataChannelTransport()

    // The Aspire's LAN IP, read from the gitignored `client-macos/.env`
    // (see LocalConfig.swift for why it stopped being a literal here).
    // Optional because that file may not exist yet on a fresh clone —
    // the view below says so instead of failing silently. No discovery
    // mechanism exists yet; the address is still typed in by hand.
    private let windowsHost = LocalConfig.windowsHost

    var body: some View {
        VStack(spacing: 16) {
            Text("Shadow Glass")
                .font(.title)
            Text(statusText)
                .foregroundStyle(.secondary)
            if windowsHost == nil {
                Text("No WINDOWS_HOST set — copy client-macos/.env.example to .env")
                    .foregroundStyle(.red)
                    .multilineTextAlignment(.center)
            }
            Button("Connect") {
                if let windowsHost {
                    transport.connect(to: windowsHost)
                }
            }
            .disabled(status != .disconnected || windowsHost == nil)
            Button("Send Hello Mac") {
                transport.send("Hello Mac")
            }
            .disabled(status != .connected)
            if !lastReceivedMessage.isEmpty {
                Text("Received: \(lastReceivedMessage)")
            }
        }
        .padding(40)
        .frame(minWidth: 300, minHeight: 200)
        .onAppear {
            transport.onStatusChanged = { status = $0 }
            transport.onMessageReceived = { lastReceivedMessage = $0 }
        }
    }

    private var statusText: String {
        switch status {
        case .disconnected: return "Not connected"
        case .connecting: return "Connecting..."
        case .connected: return "Connected"
        }
    }
}
