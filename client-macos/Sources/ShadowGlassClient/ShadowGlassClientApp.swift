import SwiftUI

// Entry point of the app. `@main` tells Swift this struct is where the
// program starts running — it replaces a traditional `main()` function
// when using SwiftUI's `App` protocol. Note: this can't live in a file
// named `main.swift` (that filename has its own implicit-entry-point
// rule that conflicts with an explicit `@main`), which is why the old
// `main.swift` placeholder is being removed in this same step.
@main
struct ShadowGlassClientApp: App {
    init() {
        LibDataChannelOfferTest.run()
    }

    var body: some Scene {
        WindowGroup {
            ContentView()
        }
    }
}

// The main (and only, for now) screen. Wired to a LowLatencyTransport —
// today a FakeTransport with no real networking, later a
// libdatachannel-backed one — through the protocol only, so this view
// never needs to change when the real implementation replaces the fake
// one.
struct ContentView: View {
    @State private var status: ConnectionStatus = .disconnected
    @State private var lastReceivedMessage = ""

    // `let` because we swap the whole transport implementation later by
    // changing this one line, not by touching anything below.
    private let transport: LowLatencyTransport = FakeTransport()

    var body: some View {
        VStack(spacing: 16) {
            Text("Shadow Glass")
                .font(.title)
            Text(statusText)
                .foregroundStyle(.secondary)
            Button("Connect") {
                transport.connect(to: "placeholder-host")
            }
            .disabled(status != .disconnected)
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
