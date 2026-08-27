# 0001 — Captura + encode por hardware (modelo Moonlight/Sunshine) em vez de RDP/FreeRDP

- Status: aceita (a parte de transporte foi substituída pelo
  [ADR 0002](0002-libwebrtc-como-transporte.md) — captura e encode abaixo
  continuam valendo)
- Data: 2026-08-27

## Contexto

O plano inicial era usar FreeRDP (cliente Swift no Mac + servidor RDP no
Windows) como base pragmática, aproveitando que o protocolo RDP já resolve
vídeo, áudio e input de forma nativa.

O uso principal do Shadow Glass, porém, é **jogos** — latência é crítica
(idealmente <80ms), inspirado em serviços como Xbox Cloud Gaming e GeForce
NOW. O servidor roda num notebook Acer Aspire com Windows 10 Home (CPU
i5-7200U + GPU dedicada NVIDIA 940MX); o cliente é um MacBook Air M1.

## Decisão

Não usar RDP/FreeRDP. Construir um pipeline de captura+encode próprio,
inspirado no par Moonlight (cliente) / Sunshine (servidor) — o mesmo padrão
do protocolo NVIDIA GameStream:

1. Captura de tela no Windows via Desktop Duplication API / Windows Graphics
   Capture
2. Encode de vídeo H.264 por hardware via **NVENC** (GPU dedicada NVIDIA
   940MX confirmada no Acer Aspire — ver ADR 0002 para o porquê de NVENC em
   vez de Intel Quick Sync)
3. ~~Transporte próprio sobre UDP~~ — **substituído por `libwebrtc`, ver
   [ADR 0002](0002-libwebrtc-como-transporte.md)**
4. Decode no Mac via VideoToolbox (hardware, nativo em Apple Silicon)
5. Canal de input (mouse/teclado) via DataChannel do `libwebrtc`
6. Áudio via captura loopback WASAPI, streaming via `libwebrtc`

## Por que não RDP/FreeRDP

- RDP foi desenhado para cenários de escritório (texto nítido, imagem
  majoritariamente estática), não para vídeo em movimento contínuo a 60fps.
- Windows só tem servidor RDP nativo nas edições Pro/Enterprise — bloqueio
  prático se o Acer Aspire rodar Windows Home. O servidor do FreeRDP existe,
  mas é a parte menos madura do projeto (o forte do FreeRDP é o cliente).
- Usar FreeRDP esconderia justamente a parte que o usuário quer aprender
  (captura, encoding, protocolo, sockets) atrás de uma lib pronta.

## Por que não WebRTC puro (revisado pelo ADR 0002)

Esta seção refletia a decisão original de adiar WebRTC para a Fase 7. O
[ADR 0002](0002-libwebrtc-como-transporte.md) reverteu isso: `libwebrtc` é
usado como transporte principal desde a Fase 2, não só no fim. Motivo da
reversão não foi técnico (a análise de complexidade abaixo continua
correta) — foi de priorização: o usuário optou por ter algo funcional
rodando ponta a ponta mais cedo, em vez de implementar transporte próprio
antes de validar o resto da pipeline.

## Consequências

- Mais trabalho de baixo nível na captura/encode (APIs nativas do Windows,
  encoder de hardware) — é o ponto, dado o objetivo de aprendizado. O
  transporte de rede não faz mais parte desse aprendizado de baixo nível
  (ver ADR 0002).
- Precisa validar na Fase 1 se NVENC funciona de fato no Acer Aspire (GPU
  940MX confirmada, mas o comportamento real do driver/SDK só se confirma
  rodando o encoder).
