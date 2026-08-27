# Diário de bordo — Shadow Glass

## 2026-08-27 — Sessão 0: decisão de arquitetura

**O que decidimos:** abandonar a ideia inicial de usar RDP/FreeRDP e construir
um pipeline próprio inspirado no modelo Moonlight/Sunshine (o mesmo padrão do
protocolo NVIDIA GameStream). Ver [ADR 0001](decisions/0001-transporte-proprio-vs-rdp.md).

**Por que isso importa:** o uso principal do projeto é jogos, com latência
crítica. RDP foi desenhado para cenários de escritório (imagem estática,
texto nítido), não para vídeo em movimento contínuo — e além disso o
servidor RDP nativo do Windows só existe nas edições Pro/Enterprise, o que
seria um bloqueio prático no Acer Aspire.

**Conceito aprendido:** a diferença entre um protocolo de "área de trabalho
remota" (RDP/VNC, otimizado para produtividade) e um protocolo de "streaming
de jogo" (Moonlight/Sunshine, GameStream — otimizado para fps alto e input
responsivo). Não são a mesma categoria de problema, mesmo parecendo
superficialmente similares ("ver e controlar outra tela").

**Próximo passo:** Fase 1 — validar no Acer Aspire se há suporte a encode de
vídeo por hardware (Intel Quick Sync), e escrever o primeiro programa em C
que captura a tela do Windows (Desktop Duplication API) sem ainda enviar
nada pela rede.

## 2026-08-27 — Sessão 3: hardware real, `libwebrtc` e um ajuste de processo

**Fato novo:** o Acer Aspire não é só GPU integrada — tem uma NVIDIA
GeForce 940MX dedicada (2GB VRAM). Isso troca o encoder de hardware pra
**NVENC** em vez de Quick Sync (é o que o Sunshine real usa quando há GPU
NVIDIA). Também confirma que os jogos-alvo (Mass Effect, LoL, Vampire:
The Masquerade – Redemption) rodam bem nessa GPU — o gargalo não deveria
ser o jogo em si.

**Decisão revertida:** ao grillar o ADR 0001, a pergunta sobre transporte
(UDP próprio vs `libwebrtc`) voltou à mesa. Optamos por `libwebrtc` desde
já (ADR 0002), não o protocolo próprio que eu tinha recomendado — prioridade
é ter algo simples e funcional rodando ponta a ponta primeiro, num projeto
que é longo prazo. A captura e o encode continuam sendo escritos à mão; só
o transporte de rede deixa de ser "from scratch".

**Lição de processo (a mais importante desta sessão):** fui repreendido,
com razão, por fazer perguntas técnicas abertas (resolução? qual encoder?)
sem antes dar contexto suficiente pra formar opinião. Pedir pra alguém
"escolher" algo que não tem repertório pra avaliar não é colaboração, é
terceirizar a decisão às cegas. Registrei a correção no `CLAUDE.md`: explicar
antes de perguntar, ou decidir e justificar quando o risco for baixo e
reversível.

**Próximo passo:** Fase 1 — primeiro programa em C/C++ no Windows: captura
de tela via Desktop Duplication API, salvando um frame como bitmap. Ainda
sem NVENC, sem rede — só validar a API de captura.

## 2026-08-27 — Fase 1: `capture_test.cpp` escrito

Primeiro código de verdade do projeto: `server-windows/src/capture_test.cpp`.
Captura um frame da tela via Desktop Duplication API e salva como
`capture.bmp`.

**Conceitos novos neste código:**
- Desktop Duplication API captura direto na GPU (rápido) em vez de GDI
  (a API antiga de screenshot, que copia pixel a pixel pela CPU).
- Frames chegam como texturas Direct3D (memória de vídeo) — não dá pra ler
  os bytes direto; precisa copiar pra uma textura "staging" antes.
- `RowPitch` (o "stride" de uma textura) pode ser maior que
  `largura * bytes_por_pixel` — a GPU alinha linhas em memória por
  performance. Ignorar isso gera uma imagem com cisalhamento.
- BMP guarda linhas de baixo pra cima — decisão de design antiga do
  formato, não bug.
- `ComPtr` (WRL) evita ter que chamar `Release()` manualmente em toda saída
  de função, inclusive nos caminhos de erro — mesma ideia de RAII que
  `std::unique_ptr`, aplicada a interfaces COM.

**Ainda não testado de verdade** — código escrito no Mac, sem acesso ao
Acer Aspire pra compilar. Próximo passo real é o usuário copiar
`server-windows/` pro Windows, buildar com CMake (ver README) e reportar
o resultado (funcionou / erro de build / etc).
