import CLibDataChannel

// A one-off check proving Swift can actually call into libdatachannel's C
// functions: initializes its logger, creates an empty PeerConnection (no
// real network config), then deletes it right away. No signaling, no
// data ever crosses the network here — this only proves the library
// links and its functions are callable. Safe to delete once the real
// transport implementation exists and exercises the library for real.
enum LibDataChannelSmokeTest {
    static func run() {
        rtcInitLogger(RTC_LOG_INFO, nil)

        var config = rtcConfiguration()
        let pc = rtcCreatePeerConnection(&config)
        if pc >= 0 {
            print("libdatachannel smoke test: created peer connection (id \(pc))")
            rtcDeletePeerConnection(pc)
        } else {
            print("libdatachannel smoke test: FAILED to create peer connection (error code \(pc))")
        }
    }
}
