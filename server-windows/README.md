# server-windows

Servidor Windows (C/C++, build via CMake).

## Fase 1: `capture_test`

Valida só a captura de tela (Desktop Duplication API), sem encode e sem
rede. Salva um frame como `capture.bmp` na pasta onde rodar.

Este código só compila no Windows (usa D3D11/DXGI). Para buildar, no
PowerShell/prompt de comando do Visual Studio, dentro de `server-windows/`:

```
cmake -B build
cmake --build build
build\Debug\capture_test.exe
```

**Verificação**: rode `capture_test.exe`, mova o mouse ou mude algo na tela
nos 5 segundos seguintes, e abra o `capture.bmp` gerado — deve mostrar a
tela real do PC no momento da captura. Se `AcquireNextFrame` der timeout,
é porque nada mudou na tela nesse intervalo (não é bug, é o comportamento
esperado da API).

Se o build falhar, cole o erro de volta na conversa com o Claude pra
debugar — ele escreve o código aqui mas não tem acesso a esta máquina pra
compilar/rodar.
