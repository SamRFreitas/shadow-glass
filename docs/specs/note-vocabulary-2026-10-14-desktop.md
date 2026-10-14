# Note — vocabulary: "desktop"

- From: the `programmer` session working on Phase 1, step 1, 2026-10-14.
- Why: "desktop" has several meanings in computing, and three of them
  have already appeared in this project. This note fixes which one is
  meant where. It lists only meanings met in the project so far; add
  one when it shows up.

## In the code: the image Windows composes

**This is the meaning in `server-windows/src/adapters_test.cpp`, in
DXGI, and in the Desktop Duplication API (Phase 1, step 2).**

The whole image Windows is drawing on the screens at a given moment:
every window, the taskbar, a full-screen game — everything visible. It
has its own coordinate system, spanning all monitors.

- `DXGI_OUTPUT_DESC::AttachedToDesktop` — true when Windows is drawing
  part of this image on that output.
- `DXGI_OUTPUT_DESC::DesktopCoordinates` — where that output sits inside
  the image. Its size depends on DPI awareness (see
  `note-learning-2026-10-13-pixels-dpi-and-an-experiment.md`).
- `int desktopAdapter` — the adapter that owns the output showing it.
- Desktop Duplication — copies this image; that is the capture of step 2.

Not the same as the desktop the user sees (wallpaper and icons): that
one is hidden behind a full-screen game, while this image includes the
game.

## The user's folder

`C:\Users\<name>\Desktop` — a folder on disk, nothing to do with the
image above.

## Remote access

As in Remote Desktop Protocol (RDP), the protocol ADR 0001 decided not
to use. Here "desktop" means the whole remote session.

## Word chosen in project text

When writing about what is captured, say "desktop image" or "the
desktop" in the first sense above. For the screen as hardware, say
"monitor"; for the DXGI object, say "output".
