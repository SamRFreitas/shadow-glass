// signaling_test.cpp
//
// Piece 10, step 1: prove a raw TCP server works on Windows, using
// Winsock2 (the native Windows sockets API), before adding JSON parsing
// or libdatachannel wiring on top (see docs/protocol.md for the full
// plan). Listens on the signaling port, accepts one connection, and just
// prints whatever text arrives — nothing smarter yet.
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

#include <cstdio>

// See docs/protocol.md — the Mac client connects to this port.
static const unsigned short SIGNALING_PORT = 45180;

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

    // Keep reading and printing until the client disconnects (recv
    // returns 0) or something goes wrong (negative return).
    char buffer[1024];
    int bytesReceived;
    while ((bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytesReceived] = '\0';
        printf("Received: %s", buffer);
    }

    printf("Client disconnected.\n");

    closesocket(clientSocket);
    closesocket(listenSocket);
    WSACleanup();
    return 0;
}
