# Shadow Glass

> "O vidro que imita o outro lado."

Acesso remoto Mac → Windows construído do zero, com foco em aprendizado
profundo de C/C++/Swift e programação de sistemas (não em usar uma lib
pronta tipo TeamViewer/AnyDesk).

## Hardware do projeto

- Cliente: MacBook Air M1 (Apple Silicon)
- Servidor: notebook Acer Aspire, Windows 10 Home Single Language.
  CPU Intel Core i5-7200U (2 núcleos/4 threads) + GPU dedicada NVIDIA
  GeForce 940MX (2GB VRAM) — encode de vídeo por hardware via **NVENC**
- Jogos-alvo: Mass Effect (trilogia), League of Legends, Vampire: The
  Masquerade – Redemption (leves/médios — a 940MX dá conta)
- Rede: LAN em casa como caso primário; acesso remoto (fora da LAN) é
  requisito futuro, não do MVP
- Resolução inicial: 720p, com arquitetura aberta para outras resoluções
  depois (não hardcodar)

## Decisão de arquitetura

Ver [`docs/decisions/0001-transporte-proprio-vs-rdp.md`](docs/decisions/0001-transporte-proprio-vs-rdp.md)
e [`docs/decisions/0002-libwebrtc-como-transporte.md`](docs/decisions/0002-libwebrtc-como-transporte.md)
(o ADR 0002 substitui a parte de transporte do ADR 0001).

Resumo: **não** usamos RDP/FreeRDP. Captura de tela + encode H.264 por
hardware (NVENC) continuam do jeito descrito no ADR 0001. O transporte de
rede, porém, usa **`libwebrtc`** desde já (ADR 0002) em vez de um protocolo
UDP próprio — decisão do usuário: priorizar algo simples e funcional rodando
ponta a ponta, mantendo a estrutura do projeto aberta para trocar o
transporte depois se necessário.

## Roteiro de fases

Cada fase entrega algo que roda de ponta a ponta, mesmo que incompleto.

0. Decisão de arquitetura + estrutura do harness (esta fase)
1. Windows: captura de tela (Desktop Duplication API) + encode H.264 via
   NVENC, validado localmente (sem rede ainda)
2. Rede: integrar `libwebrtc` no servidor Windows, transporte de vídeo via
   LAN até um receptor simples
3. Mac: cliente Swift mínimo recebendo o stream via `libwebrtc`,
   decodificando (VideoToolbox) e desenhando na tela
4. Canal de input: mouse/teclado do Mac → DataChannel → injeção no Windows
   (`SendInput`)
5. Áudio: captura loopback (WASAPI) → `libwebrtc` → playback no Mac
6. Tuning de latência, perda de pacote, bitrate adaptativo
7. Acesso remoto fora da LAN — já vem de graça do ICE/STUN/TURN do
   `libwebrtc` (ver ADR 0002); só falta configurar/testar

## Estrutura do repositório

- `client-macos/` — pacote Swift Package Manager (não `.xcodeproj` cru — ver
  "Cliente Mac: SPM em vez de Xcode project" abaixo)
- `server-windows/` — código C/C++, build via CMake (código chega na Fase 1)
- `docs/decisions/` — ADRs (Architecture Decision Records), uma decisão por
  arquivo, numeradas. Puro Markdown, sem nada específico de Claude.
- `docs/LEARNING_LOG.md` — diário de bordo: uma entrada por sessão de
  trabalho, registrando o que foi aprendido e por quê.
- `docs/protocol.md` — a criar na Fase 2: como o servidor e o cliente se
  encontram (sinalização) e o formato dos payloads trocados via DataChannel
  do `libwebrtc`. É o único "contrato compartilhado" entre C/C++ e Swift.
- `.claude/skills/` — ainda não existe. Só criaremos skills quando houver um
  script real e repetido para orquestrar (build, rodar servidor, testar
  conexão). Cada skill será um Markdown com frontmatter (nome, descrição,
  quando usar) chamando um script em `scripts/*.sh` — a lógica mecânica fica
  no script, a skill só decide quando/por quê rodá-lo. Isso mantém tudo
  executável fora do Claude Code (bash puro).
- MCP não é usado neste projeto — não há integração de ferramenta externa
  que justifique isso ainda.

## Cliente Mac: SPM em vez de Xcode project

O cliente Mac usa **Swift Package Manager**, não um `.xcodeproj` tradicional.
Motivo prático: um projeto Xcode clássico guarda a lista de arquivos num
`project.pbxproj` (referências por caminho/UUID); um `.swift` criado direto
no disco (por mim ou por qualquer editor) não entra automaticamente na
build sem um passo manual de "Add Files to Project" no Xcode. SPM descobre
arquivos por convenção de pasta (`Sources/ShadowGlassClient/*.swift`), sem
esse arquivo de projeto pra manter sincronizado. Ainda abre normal no Xcode
(basta abrir `Package.swift`) e também builda por `swift build`/`swift run`
no terminal, sem precisar da GUI.

## Processo de decisão

Decisões de arquitetura relevantes (o que vira um ADR) passam antes pela
skill `grilling` — usada para estressar a decisão (alternativas descartadas,
reversibilidade, o que quebra) antes de ela ser registrada como definitiva
em `docs/decisions/`. Isso é uma convenção de processo, não uma dependência
de implementação: mesmo sem a skill instalada, o mesmo processo pode ser
seguido manualmente (perguntar "o que descartei e por quê", "isso é
reversível", "o que quebra se eu escolher diferente" antes de fechar um ADR).

## Abordagem pedagógica

Este projeto prioriza aprendizado sobre velocidade. Explicações do "porquê"
acompanham as mudanças de código; código novo vem com comentários que
explicam motivação, não só mecânica óbvia.

## Observação de processo (feedback do usuário, 2026-08-27)

O usuário concorda com o princípio de perguntar antes de decidir, mas
apontou que perguntas técnicas abertas (ex: "escolha resolução/fps",
"escolha encoder") sem contexto prévio não funcionam quando ele ainda não
tem repertório pra avaliar as opções — isso vira decisão às cegas, não
colaboração.

Regra daqui pra frente: quando uma decisão exigir conhecimento que o
usuário ainda não tem, explicar o conceito e o trade-off primeiro (o que é,
pra que serve, o que muda cada escolha) antes de perguntar — ou, se a
decisão for de baixo risco e reversível, já tomar a decisão com justificativa
e seguir, deixando espaço pra ele vetar depois, em vez de bloquear esperando
uma resposta que ele não tem base pra dar.
