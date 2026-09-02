cmake_minimum_required(VERSION 3.20)
project(shadow_glass_server LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "server-windows only compiles on Windows (it uses the Desktop Duplication API / D3D11, which are Windows APIs)")
endif()

# Phase 1: just the screen capture test. More executables (encode, network)
# will land as new add_executable() calls in later phases — no anticipating
# structure that doesn't exist yet.
add_executable(capture_test src/capture_test.cpp)
target_link_libraries(capture_test PRIVATE d3d11 dxgi)
set_property(TARGET capture_test PROPERTY CXX_STANDARD 17)

# Connectivity test, piece 7: build libdatachannel as part of our own
# project (not just standalone in third_party/) and link a real executable
# against it. NO_EXAMPLES/NO_TESTS off the default ON, since we only need
# the library itself here, not its bundled example programs/test suite —
# skipping those cuts build time down noticeably.
set(NO_EXAMPLES ON CACHE BOOL "" FORCE)
set(NO_TESTS ON CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../third_party/libdatachannel
                  ${CMAKE_CURRENT_BINARY_DIR}/libdatachannel)

add_executable(datachannel_offer_test src/datachannel_offer_test.cpp)
target_link_libraries(datachannel_offer_test PRIVATE datachannel)
set_property(TARGET datachannel_offer_test PROPERTY CXX_STANDARD 17)

# datachannel is built as a DLL (BUILD_SHARED_LIBS defaults ON), and it
# lands in a different build subfolder than datachannel_offer_test.exe
# because of how add_subdirectory() lays out per-target output. Windows
# only looks for a DLL next to the .exe (or on PATH) when you just
# double-click it, not in other build folders — so without this, running
# the .exe directly fails with "datachannel.dll was not found". This copies
# the freshly built DLL next to the .exe after every build, automatically.
add_custom_command(TARGET datachannel_offer_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:datachannel>
        $<TARGET_FILE_DIR:datachannel_offer_test>
)

# Piece 10: a Winsock2 TCP server reading the wire format from
# docs/protocol.md, still isolated from libdatachannel's PeerConnection.
# The JSON header include path points at nlohmann/json's copy already
# vendored inside the libdatachannel submodule — not a new dependency.
add_executable(signaling_test src/signaling_test.cpp)
target_link_libraries(signaling_test PRIVATE ws2_32)
target_include_directories(signaling_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/libdatachannel/deps/json/include)
set_property(TARGET signaling_test PROPERTY CXX_STANDARD 17)
