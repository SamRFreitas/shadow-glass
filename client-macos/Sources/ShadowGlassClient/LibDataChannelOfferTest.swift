import CLibDataChannel
import Darwin

// Piece 5: drive the PeerConnection to actually negotiate. Creating a
// DataChannel, as the side that initiates it, makes libdatachannel start
// generating a local description ("offer") on its own — we just print it
// to the console when the callback fires. Nothing is sent over the
// network yet; this only proves we can drive the negotiation correctly,
// which the real signaling (next pieces) depends on. Supersedes and
// replaces LibDataChannelSmokeTest, which only proved the bridge itself
// worked.
final class LibDataChannelOfferTest {
    static func run() {
        // Kept alive on purpose: this object owns the peer connection's
        // lifetime for as long as the app runs (this is a throwaway test,
        // not the real transport, so there's no cleanup path yet).
        let test = LibDataChannelOfferTest()
        test.start()
    }

    private var pc: Int32 = -1

    private func start() {
        // stdout is "fully buffered" (not flushed until the buffer fills
        // or the program exits) whenever it isn't attached to an actual
        // terminal — e.g. when it's redirected to a file or a pipe, which
        // is exactly what happens whenever this app's output is captured
        // for testing. Without this, a handful of print() calls can sit
        // invisible in memory for a long time. Line-buffered mode flushes
        // after every newline instead, so our prints show up immediately.
        setvbuf(stdout, nil, _IOLBF, 0)

        rtcInitLogger(RTC_LOG_WARNING, nil)

        var config = rtcConfiguration()
        pc = rtcCreatePeerConnection(&config)
        guard pc >= 0 else {
            print("offer test: failed to create peer connection (\(pc))")
            return
        }

        // The C callback below is a plain function pointer — it can't
        // capture `self` the way a JavaScript arrow function captures its
        // surrounding scope. `Unmanaged` is Swift's way of turning `self`
        // into the kind of raw pointer C expects, so we can hand it to
        // rtcSetUserPointer and get it back, unchanged, as the `ptr`
        // argument every time a callback fires.
        let context = Unmanaged.passUnretained(self).toOpaque()
        rtcSetUserPointer(pc, context)

        rtcSetLocalDescriptionCallback(pc) { _, sdp, type, ptr in
            guard let ptr, let sdp, let type else { return }
            let instance = Unmanaged<LibDataChannelOfferTest>.fromOpaque(ptr).takeUnretainedValue()
            instance.handleLocalDescription(sdp: String(cString: sdp), type: String(cString: type))
        }

        // We're the offering side, so creating a data channel is what
        // kicks off negotiation (matches libdatachannel's own
        // examples/copy-paste-capi/offerer.c reference).
        let dc = rtcCreateDataChannel(pc, "shadow-glass")
        if dc < 0 {
            print("offer test: failed to create data channel (\(dc))")
        }
    }

    private func handleLocalDescription(sdp: String, type: String) {
        print("=== local SDP (\(type)) ===")
        print(sdp)
        print("============================")
    }
}
