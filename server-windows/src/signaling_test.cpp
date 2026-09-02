// signaling_test.cpp
//
// Piece 10: a raw TCP server on Windows, using Winsock2 (the native
// Windows sockets API). Step 1 proved the socket itself works; this
// step reads whatever arrives and interprets it as the newline-delimited
// JSON messages docs/protocol.md defines (offer/answer/candidate) —
// still with no libdatachannel wiring, on purpose: proving we can read
// the right message before adding a real PeerConnection on top.
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

// Not a new dependency: this header already lives in the repo, vendored
// as one of libdatachannel's own submodules (third_party/libdatachannel/
// deps/json) — the include path below just points our own code at it too.
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <cstdio>
#include <sstream>
#include <string>

// See docs/protocol.md — the Mac client connects to this port.
static const unsigned short SIGNALING_PORT = 45180;

// Parses one line of the wire format and prints what it recognized.
// ponytail: this only reports what it sees — nothing here calls into
// libdatachannel yet, that's the next piece.
static void handleMessage(const std::string& line) {
    if (line.empty()) return;

    json message;
    try {
        message = json::parse(line);
    } catch (const json::parse_error& e) {
        printf("  Could not parse as JSON: %s\n", e.what());
        return;
    }

    const std::string type = message.value("type", "");
    if (type == "offer" || type == "answer") {
        printf("  Recognized a '%s' message (%zu bytes of SDP)\n",
               type.c_str(), message.value("sdp", std::string()).size());
    } else if (type == "candidate") {
        printf("  Recognized a 'candidate' message: %s (mid=%s)\n",
               message.value("candidate", std::string()).c_str(),
               message.value("mid", std::string()).c_str());
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
            handleMessage(line);
        }
    }

    printf("Client disconnected.\n");
    if (!accumulated.empty()) {
        printf("(leftover data with no trailing newline — handling anyway)\n");
        handleMessage(accumulated);
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
