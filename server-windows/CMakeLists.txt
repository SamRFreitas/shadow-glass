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
