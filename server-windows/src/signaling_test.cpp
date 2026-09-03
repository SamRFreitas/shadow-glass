// signaling_test.cpp
//
// Piece 10 built a raw TCP server on Windows, using Winsock2 (the native
// Windows sockets API), that reads the newline-delimited JSON messages
// docs/protocol.md defines (offer/answer/candidate) — but only printed
// what it saw, with no libdatachannel wiring, on purpose: proving we
// could read the right message before adding a real PeerConnection on
// top of it.
//
// Piece 11 (this version) wires it up for real: this program is always
// the *answerer* (docs/protocol.md's exchange sequence has the Mac send
// the offer first) — when a real 'offer' arrives, it hands it to a
// PeerConnection, which reacts by generating our own 'answer' and our
// own ICE candidates asynchronously; those get written back over the
// same TCP socket instead of just printed. This is the same role the
// library's own answerer.exe example played by hand in piece 8, now
// automated end to end.
//
// Winsock2 is conceptually the same as the BSD/POSIX sockets API Unix
// systems (macOS included) use — socket(), bind(), listen(), accept(),
// recv() all exist there too, since Microsoft modeled Winsock after the
// same ideas in the early 90s. The differences worth knowing: Winsock
// needs an explicit startup/cleanup call (WSAStartup/WSACleanup) before
// any socket function works at all — POSIX systems need no equivalent,
// sockets are just always available — and it reports errors via
// WSAGetLastError() instead of POSIX's errno.

#include <winsock2.h>
#include <ws2tcpip.h>

#include "rtc/rtc.hpp"

// Not a new dependency: this header already lives in the repo, vendored
// as one of libdatachannel's own submodules (third_party/libdatachannel/
// deps/json) — the include path below just points our own code at it too.
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <cstdio>
#include <memory>
#include <sstream>
#include <string>

// See docs/protocol.md — the Mac client connects to this port.
static const unsigned short SIGNALING_PORT = 45180;

// Writes one JSON message back to the Mac, in the same newline-delimited
// format we read on the way in — this is the write side of the same wire
// format piece 10 only ever read.
static void sendJson(SOCKET clientSocket, const json& message) {
    std::string line = message.dump() + "\n";
    send(clientSocket, line.c_str(), static_cast<int>(line.size()), 0);
}

// Parses one line of the wire format and, now, acts on it: feeds an
// offer/candidate to the real PeerConnection instead of just printing it.
// The PeerConnection reacts on its own (asynchronously, via the callbacks
// set up in main()) by producing our answer and our own candidates.
static void handleMessage(const std::string& line, rtc::PeerConnection& pc) {
    if (line.empty()) return;

    json message;
    try {
        message = json::parse(line);
    } catch (const json::parse_error& e) {
        printf("  Could not parse as JSON: %s\n", e.what());
        return;
    }

    const std::string type = message.value("type", "");
    if (type == "offer") {
        std::string sdp = message.value("sdp", std::string());
        printf("  Recognized an 'offer' message (%zu bytes of SDP) -- handing it to the PeerConnection\n",
               sdp.size());
        pc.setRemoteDescription(rtc::Description(sdp, "offer"));
    } else if (type == "candidate") {
        std::string candidate = message.value("candidate", std::string());
        std::string mid = message.value("mid", std::string());
        printf("  Recognized a 'candidate' message: %s (mid=%s) -- adding it to the PeerConnection\n",
               candidate.c_str(), mid.c_str());
        pc.addRemoteCandidate(rtc::Candidate(candidate, mid));
    } else {
        printf("  Unrecognized message type: '%s'\n", type.c_str());
    }
}

int main() {
    // "Loads" Winsock. Nothing else in this file can run before this
    // succeeds.
    WSADATA wsaData;
    int startupResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (startupResult != 0) {
        fprintf(stderr, "WSAStartup failed: %d\n", startupResult);
        return 1;
    }

    // A TCP socket (SOCK_STREAM), IPv4 (AF_INET) — matches the "host"
    // candidates we saw during the connectivity test; this signaling
    // channel is LAN-only, so IPv6 isn't needed here.
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        fprintf(stderr, "socket() failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Bind to INADDR_ANY (all of this machine's network interfaces) on
    // our signaling port — this is what makes the Mac able to reach it
    // via the Aspire's real LAN IP, not just from the same machine.
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(SIGNALING_PORT); // htons: host byte order -> network byte order

    if (bind(listenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        fprintf(stderr, "bind() failed: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    // Backlog of 1: only ever expect one Mac client waiting to connect
    // at a time — plenty for this project.
    if (listen(listenSocket, 1) == SOCKET_ERROR) {
        fprintf(stderr, "listen() failed: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    printf("Listening on port %d. Waiting for a connection...\n", SIGNALING_PORT);

    // Blocks here until something actually connects.
    SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);
    if (clientSocket == INVALID_SOCKET) {
        fprintf(stderr, "accept() failed: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    printf("Client connected. Reading data...\n");

    // Warning level only — we don't need the library's own internal trace,
    // just our own output (same choice as datachannel_offer_test.cpp).
    rtc::InitLogger(rtc::LogLevel::Warning);

    rtc::Configuration config;
    rtc::PeerConnection pc(config);

    // Fires once the library has generated our side's answer, which
    // happens automatically after setRemoteDescription() below sees an
    // offer — we never call createDataChannel() or ask for an offer
    // ourselves, since answering (not offering) is this program's fixed
    // role in docs/protocol.md's exchange.
    pc.onLocalDescription([clientSocket](rtc::Description description) {
        json out;
        out["type"] = description.typeString(); // "answer"
        out["sdp"] = std::string(description);
        sendJson(clientSocket, out);
        printf("Sent our '%s' back to the Mac.\n", description.typeString().c_str());
    });

    // Fires once per ICE candidate our side discovers — each one is sent
    // back the moment it's found, not batched, matching the "no end-of-
    // candidates marker" simplification docs/protocol.md already documents.
    pc.onLocalCandidate([clientSocket](rtc::Candidate candidate) {
        json out;
        out["type"] = "candidate";
        out["candidate"] = candidate.candidate();
        out["mid"] = candidate.mid();
        sendJson(clientSocket, out);
        printf("Sent one of our candidates back to the Mac.\n");
    });

    // Fires when the Mac's DataChannel actually reaches us — this is the
    // payoff of the whole exchange: proof the negotiation above actually
    // worked, without a human copy-pasting anything (piece 8's manual test,
    // now automatic).
    pc.onDataChannel([](std::shared_ptr<rtc::DataChannel> dc) {
        printf("DataChannel '%s' received from the Mac!\n", dc->label().c_str());
        dc->onOpen([]() { printf("DataChannel is open.\n"); });
        dc->onMessage([](rtc::message_variant data) {
            if (std::holds_alternative<std::string>(data)) {
                printf("Message from Mac: %s\n", std::get<std::string>(data).c_str());
            }
        });
    });

    // TCP only guarantees a stream of bytes, not message boundaries — one
    // recv() call might return half a line, or several lines glued
    // together. Accumulating into a string and pulling out each complete
    // line (up to '\n') handles both cases correctly.
    std::string accumulated;
    char buffer[1024];
    int bytesReceived;
    while ((bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        accumulated.append(buffer, bytesReceived);

        size_t newlinePos;
        while ((newlinePos = accumulated.find('\n')) != std::string::npos) {
            std::string line = accumulated.substr(0, newlinePos);
            accumulated.erase(0, newlinePos + 1);
            printf("Received line: %s\n", line.c_str());
            handleMessage(line, pc);
        }
    }

    printf("Client disconnected.\n");
    if (!accumulated.empty()) {
        printf("(leftover data with no trailing newline -- handling anyway)\n");
        handleMessage(accumulated, pc);
    }

    closesocket(clientSocket);
    closesocket(listenSocket);
    WSACleanup();

    // Waits for a keypress before the program actually exits. Without
    // this, double-clicking the .exe in Explorer (rather than running it
    // from an already-open terminal) closes this whole console window the
    // instant main() returns — before there's any chance to read what was
    // just printed above. A .bat script's own `pause` command doesn't
    // help here, since that only protects the case where a .bat launched
    // it; this protects every way the program can be started.
    printf("\nPress Enter to exit...");
    getchar();

    return 0;
}
