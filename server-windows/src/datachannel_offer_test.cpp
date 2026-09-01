// datachannel_offer_test.cpp
//
// Windows side of the connectivity test, piece 7: prove this project's own
// C++ code can drive libdatachannel and generate an SDP offer — the exact
// same test the Mac side already did (LibDataChannelOfferTest.swift), just
// in native C++ instead of through a Swift/C bridge. Still no networking:
// nothing is sent anywhere, this only validates the library is wired into
// our own build correctly.
//
// Unlike the Mac side, C++ doesn't need any bridging trick to talk to this
// library: it links directly against libdatachannel's own C++ API
// (rtc::PeerConnection, rtc::DataChannel), and callbacks are plain lambdas
// that can capture whatever they need — no "user pointer" workaround
// required, since that C-only limitation doesn't apply here.

#include "rtc/rtc.hpp"

#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

int main() {
    // Warning level only — we don't need the library's own internal trace,
    // just our own output.
    rtc::InitLogger(rtc::LogLevel::Warning);

    rtc::Configuration config;
    auto pc = std::make_shared<rtc::PeerConnection>(config);

    // Fires once the library has generated our side's SDP offer.
    pc->onLocalDescription([](rtc::Description description) {
        std::cout << "=== local SDP (" << description.typeString() << ") ===" << std::endl;
        std::cout << std::string(description) << std::endl;
        std::cout << "============================" << std::endl;
    });

    // Creating a data channel, as the side that initiates it, is what
    // makes libdatachannel start negotiating and produce an offer —
    // same reasoning as the Mac side.
    auto dc = pc->createDataChannel("shadow-glass");

    // Give the library a moment to generate and print the offer before the
    // process exits.
    std::this_thread::sleep_for(std::chrono::seconds(2));

    return 0;
}
