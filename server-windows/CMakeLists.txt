cmake_minimum_required(VERSION 3.20)
project(shadow_glass_server LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "server-windows so compila no Windows (usa Desktop Duplication API / D3D11, que sao APIs do Windows)")
endif()

# Fase 1: só o teste de captura de tela. Mais executáveis (encode, rede)
# entram como novos add_executable() nas próximas fases — nada de
# antecipar estrutura que ainda não existe.
add_executable(capture_test src/capture_test.cpp)
target_link_libraries(capture_test PRIVATE d3d11 dxgi)
set_property(TARGET capture_test PROPERTY CXX_STANDARD 17)
