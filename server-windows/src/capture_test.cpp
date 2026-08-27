// capture_test.cpp
//
// FASE 1 do Shadow Glass: validar a captura de tela no Windows, sozinha,
// sem encode e sem rede. Se este programa rodar e gerar um capture.bmp
// que abre normal e mostra a tela real do PC, a peça mais fundamental do
// projeto (conseguir "ver" a tela por baixo dos panos) está funcionando.
//
// A API usada é a Desktop Duplication API (Windows 8+), acessada através
// do DXGI (DirectX Graphics Infrastructure). É a mesma classe de API que
// ferramentas de streaming/gravação de tela profissionais usam, porque
// captura direto na GPU (rápido, baixa latência) em vez de fazer
// screenshot via GDI (a API antiga, lenta, que copia pixel a pixel pela
// CPU). Entender esse programa é entender o primeiro terço do pipeline
// descrito no ADR 0001.

#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h> // Microsoft::WRL::ComPtr — ponteiro "inteligente" pra
                         // interfaces COM. Sem ele, teríamos que chamar
                         // ->Release() manualmente em cada objeto, em toda
                         // saída de função (inclusive nos caminhos de erro) —
                         // ComPtr faz isso sozinho quando sai de escopo,
                         // igual um std::unique_ptr faria pra memória comum.

#include <cstdio>
#include <cstdint>
#include <cstdlib>

using Microsoft::WRL::ComPtr;

// A maioria das funções do Direct3D/DXGI devolve um HRESULT (um código de
// erro de 32 bits) em vez de lançar exceções. É assim que a maior parte do
// Windows "antigo" (COM) sinaliza erro. Essa função só existe pra não
// repetir "if (FAILED(hr)) { ... }" depois de toda chamada — em código de
// produção você trataria cada erro de forma diferente, mas aqui, pra um
// teste de validação, "falhou = aborta e mostra onde" já é suficiente.
static void CheckHR(HRESULT hr, const char* etapa) {
    if (FAILED(hr)) {
        fprintf(stderr, "Falhou em '%s' (HRESULT 0x%08lx)\n", etapa, hr);
        exit(1);
    }
}

// Escreve os pixels capturados como um arquivo .bmp.
//
// BMP é propositalmente o formato mais simples que existe: um cabeçalho de
// arquivo (BITMAPFILEHEADER), um cabeçalho de imagem (BITMAPINFOHEADER) e
// os pixels crus em seguida, sem nenhuma compressão. Ambos os structs já
// vêm definidos em <windows.h> (em wingdi.h) — não precisamos declarar nada
// na mão.
//
// Um detalhe que sempre pega gente de surpresa na primeira vez: o BMP
// guarda as linhas de baixo pra cima (a última linha da imagem vem
// primeiro no arquivo) quando biHeight é positivo. O Desktop Duplication
// API entrega os dados de cima pra baixo, como qualquer imagem normal.
// Em vez de inverter os bytes na memória, é mais simples só escrever as
// linhas na ordem inversa direto no arquivo.
static void SaveAsBmp(const char* caminho, const D3D11_MAPPED_SUBRESOURCE& mapeado,
                      UINT largura, UINT altura) {
    const UINT bytesPorPixel = 4; // o formato do frame é BGRA: 1 byte por canal, 4 canais
    const UINT bytesPorLinha = largura * bytesPorPixel; // 32bpp já é múltiplo de 4, sem padding
    const DWORD tamanhoPixels = bytesPorLinha * altura;

    BITMAPFILEHEADER fileHeader{};
    fileHeader.bfType = 0x4D42; // as letras "BM" em little-endian — assinatura obrigatória do formato
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    fileHeader.bfSize = fileHeader.bfOffBits + tamanhoPixels;

    BITMAPINFOHEADER infoHeader{};
    infoHeader.biSize = sizeof(BITMAPINFOHEADER);
    infoHeader.biWidth = static_cast<LONG>(largura);
    infoHeader.biHeight = static_cast<LONG>(altura); // positivo => "bottom-up", o padrão do formato
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 32;
    infoHeader.biCompression = BI_RGB; // sem compressão
    infoHeader.biSizeImage = tamanhoPixels;

    FILE* arquivo = fopen(caminho, "wb");
    if (!arquivo) {
        fprintf(stderr, "Nao consegui criar o arquivo %s\n", caminho);
        exit(1);
    }

    fwrite(&fileHeader, sizeof(fileHeader), 1, arquivo);
    fwrite(&infoHeader, sizeof(infoHeader), 1, arquivo);

    // mapeado.RowPitch é o "stride" real da textura na GPU — quantos bytes
    // separam o início de uma linha do início da próxima. Isso NEM SEMPRE
    // é igual a largura*4: a GPU costuma alinhar linhas em múltiplos de,
    // por exemplo, 256 bytes, por motivos de performance de memória. Se a
    // gente ignorasse o RowPitch e assumisse "largura*4 bytes por linha",
    // a imagem sairia com um efeito de "cisalhamento" (linhas deslocadas)
    // sempre que a largura real não bater com esse alinhamento.
    const uint8_t* base = static_cast<const uint8_t*>(mapeado.pData);
    for (LONG linha = static_cast<LONG>(altura) - 1; linha >= 0; --linha) {
        const uint8_t* linhaAtual = base + static_cast<size_t>(linha) * mapeado.RowPitch;
        fwrite(linhaAtual, bytesPorLinha, 1, arquivo);
    }

    fclose(arquivo);
}

int main() {
    HRESULT hr;

    // PASSO 1 — Criar um "device" Direct3D 11.
    // Isso é literalmente uma conexão com a GPU: é através dele que
    // pedimos recursos (texturas), copiamos dados, etc. Precisamos disso
    // porque o Desktop Duplication API entrega os frames como texturas
    // Direct3D (dados que vivem na memória de vídeo), não como um array
    // de bytes comum em RAM.
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> contexto;
    D3D_FEATURE_LEVEL nivelDeSuporte;
    hr = D3D11CreateDevice(
        nullptr,                    // usar o adaptador de vídeo padrão do sistema
        D3D_DRIVER_TYPE_HARDWARE,   // queremos a GPU de verdade, não um driver de software
        nullptr,
        0,                           // nenhuma flag especial (ex: modo debug)
        nullptr, 0,                  // deixa o Windows escolher o "feature level" mais alto disponível
        D3D11_SDK_VERSION,
        &device,
        &nivelDeSuporte,
        &contexto);
    CheckHR(hr, "D3D11CreateDevice");

    // PASSO 2 — Achar o "output" (monitor) que queremos duplicar.
    // A cadeia é meio burocrática porque o D3D11 e o DXGI são APIs
    // separadas que conversam entre si via COM: pegamos o device DXGI por
    // trás do device D3D11, dele pegamos o adaptador de vídeo (a placa de
    // vídeo em si), e do adaptador pegamos o output de índice 0 (na
    // prática, o monitor principal na maioria das configurações).
    ComPtr<IDXGIDevice> dxgiDevice;
    hr = device.As(&dxgiDevice);
    CheckHR(hr, "device.As<IDXGIDevice>");

    ComPtr<IDXGIAdapter> adaptador;
    hr = dxgiDevice->GetAdapter(&adaptador);
    CheckHR(hr, "GetAdapter");

    ComPtr<IDXGIOutput> output;
    hr = adaptador->EnumOutputs(0, &output);
    CheckHR(hr, "EnumOutputs(0)");

    // A capacidade de "duplicar" um output só existe na interface
    // IDXGIOutput1 (uma versão estendida de IDXGIOutput, introduzida
    // junto com o próprio Desktop Duplication API) — por isso mais um
    // QueryInterface (o .As<>() do ComPtr) aqui.
    ComPtr<IDXGIOutput1> output1;
    hr = output.As(&output1);
    CheckHR(hr, "output.As<IDXGIOutput1>");

    // PASSO 3 — Pedir a duplicação.
    // A partir deste ponto, o Windows nos entrega uma "cópia ao vivo" de
    // tudo que está sendo desenhado nesse monitor. É o coração de toda a
    // Fase 1 — sem isso não tem captura de tela nenhuma.
    ComPtr<IDXGIOutputDuplication> duplicacao;
    hr = output1->DuplicateOutput(device.Get(), &duplicacao);
    CheckHR(hr, "DuplicateOutput");

    printf("Duplicacao criada. Mude algo na tela (mova o mouse, etc) nos proximos 5 segundos...\n");

    // PASSO 4 — Pegar o próximo frame disponível.
    // AcquireNextFrame só devolve um frame novo quando a tela realmente
    // mudou desde a última chamada (é uma otimização: por que reenviar um
    // frame idêntico ao anterior?). O timeout de 5000ms é só pra este
    // teste manual — dá tempo da gente mexer o mouse antes de desistir.
    DXGI_OUTDUPL_FRAME_INFO infoFrame;
    ComPtr<IDXGIResource> recursoDesktop;
    hr = duplicacao->AcquireNextFrame(5000, &infoFrame, &recursoDesktop);
    CheckHR(hr, "AcquireNextFrame");

    // O frame vem como um IDXGIResource genérico; na prática, o tipo real
    // por trás dele é sempre uma ID3D11Texture2D (uma textura 2D comum).
    ComPtr<ID3D11Texture2D> frameNaGpu;
    hr = recursoDesktop.As(&frameNaGpu);
    CheckHR(hr, "recursoDesktop.As<ID3D11Texture2D>");

    D3D11_TEXTURE2D_DESC descFrame;
    frameNaGpu->GetDesc(&descFrame);

    // PASSO 5 — Copiar pra uma textura "staging" (que a CPU consegue ler).
    // `frameNaGpu` foi criada só pra uso da própria GPU — a CPU não tem
    // permissão de ler os bytes dela diretamente. Uma textura "staging" é
    // uma textura normal, mas criada com uma flag que permite Map/Unmap
    // (ou seja, "emprestar" a memória pra CPU ler por um instante).
    D3D11_TEXTURE2D_DESC descStaging = descFrame;
    descStaging.Usage = D3D11_USAGE_STAGING;
    descStaging.BindFlags = 0; // staging não pode ser usada como render target/shader input
    descStaging.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    descStaging.MiscFlags = 0;

    ComPtr<ID3D11Texture2D> texturaStaging;
    hr = device->CreateTexture2D(&descStaging, nullptr, &texturaStaging);
    CheckHR(hr, "CreateTexture2D(staging)");

    contexto->CopyResource(texturaStaging.Get(), frameNaGpu.Get());

    // PASSO 6 — Mapear a textura staging e ler os pixels de verdade.
    D3D11_MAPPED_SUBRESOURCE mapeado;
    hr = contexto->Map(texturaStaging.Get(), 0, D3D11_MAP_READ, 0, &mapeado);
    CheckHR(hr, "Map(staging)");

    SaveAsBmp("capture.bmp", mapeado, descFrame.Width, descFrame.Height);

    contexto->Unmap(texturaStaging.Get(), 0);

    // Devolve o frame pro sistema. Sem isso, o Windows guarda a memória
    // ocupada esperando a gente "terminar" com ela — em loop de captura de
    // verdade (Fase 2 em diante), isso teria que rodar a cada frame.
    duplicacao->ReleaseFrame();

    printf("Frame capturado: %ux%u -> capture.bmp\n", descFrame.Width, descFrame.Height);
    return 0;
}
