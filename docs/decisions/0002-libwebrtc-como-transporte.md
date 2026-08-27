# 0002 — Adotar `libwebrtc` como transporte principal (substitui a parte de transporte do ADR 0001)

- Status: aceita
- Data: 2026-08-27
- Substitui: a seção de transporte do [ADR 0001](0001-transporte-proprio-vs-rdp.md)
  (captura de tela e encode por hardware do ADR 0001 continuam válidos e
  não são afetados por esta decisão)

## Contexto

O ADR 0001 propunha um transporte UDP próprio (protocolo caseiro, sem lib
pronta) para maximizar aprendizado de baixo nível (sockets, framing,
confiabilidade manual). Ao revisitar a decisão antes da Fase 1, o usuário
apontou que:

- O projeto é longo prazo — prioridade é ter algo simples e funcional
  rodando ponta a ponta primeiro, não implementar tudo do zero de uma vez.
- Um transporte UDP próprio adiciona um projeto de sistemas inteiro (framing,
  retransmissão seletiva, controle de congestionamento) antes mesmo de
  validar o resto da pipeline (captura, encode, decode, renderização).

## Decisão

Usar `libwebrtc` como transporte de rede desde a Fase 2, em vez de um
protocolo UDP próprio. `libwebrtc` já entrega, prontos: transporte de vídeo/
áudio de baixa latência, DataChannel (pra eventos de input), criptografia
(DTLS/SRTP) e NAT traversal (ICE/STUN/TURN) — o que também adianta o
requisito de acesso remoto fora da LAN (Fase 7), sem precisar de uma segunda
implementação de transporte depois.

A captura de tela e o encode por hardware (NVENC, ver ADR 0001) continuam
sendo escritos à mão em C/C++ — é ali que o aprendizado de baixo nível deste
projeto se concentra. `libwebrtc` entra só como camada de transporte,
mantida desacoplada da captura/encode por uma interface própria, para que
seja possível trocar o transporte no futuro sem reescrever o resto do
pipeline (ex: se um dia fizer sentido migrar para um protocolo próprio, só
essa camada muda).

## Consequências

- Menos código de rede escrito à mão — o aprendizado nessa camada passa a
  ser "como integrar/configurar uma stack de produção real", não "como
  implementar um protocolo do zero".
- Ganha de graça: acesso remoto fora da LAN (NAT traversal), criptografia,
  e um canal de dados pronto pra input — sem retrabalho na Fase 7.
- Custo: `libwebrtc` é uma codebase C++ grande; a curva de integração
  inicial (build, APIs de PeerConnection, sinalização) é maior que abrir um
  socket UDP cru.
- Fica como responsabilidade de design manter uma fronteira clara entre
  "pipeline de captura/encode" e "camada de transporte", para que a escolha
  de transporte permaneça substituível.
