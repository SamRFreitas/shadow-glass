# Note — what was learned on 2026-10-06/07: DXGI vocabulary and pointers

- From: the `programmer` session working on Phase 1, step 1.
- Raw material for a `docs/LEARNING_LOG.md` entry; not a spec.

## Facts confirmed on the Acer

- **Piece 1 works.** `adapters_test.exe` printed `DXGI factory created.`
- **Windows build:** `10.0.19045.6466` (Windows 10, version 22H2). No
  DXGI API considered for step 2 is ruled out by the Windows version.
- **Device Manager → "Adaptadores de vídeo"** lists the Intel and the
  NVIDIA adapters. Not yet compared with our program's own listing.
- **The repository moved.** OneDrive had redirected the Desktop folder
  to `C:\Users\samue\OneDrive\Desktop`; the old `build` folder kept the
  previous absolute path in `CMakeCache.txt` and CMake refused to
  configure. The project was cloned again at
  `C:\Users\samue\shadow-glass`, outside OneDrive. A CMake `build`
  folder cannot be moved or reused after its project changes path.

## Where DXGI sits

The picture that made the layers click:

```
our program
      ↓  asks
    DXGI            (part of Windows)
      ↓
   drivers          (Intel's, NVIDIA's)
      ↓
   the GPUs         (the hardware)
```

Our program never talks to a card directly. It asks DXGI; DXGI knows
what exists because it talks to the drivers; the drivers deal with what
is different between one manufacturer and another. That is why the code
has no "if Intel / if NVIDIA" line. Device Manager and DXGI are two
windows onto the same information: one for people, one for programs.

Inside DXGI, the objects are only handed out through a chain, and step 1
walks it one link per piece:

```
factory   →   adapter          →   output
(counter)     (graphics card)      (screen)
```

## Vocabulary

| Term | What it really is | On the Acer |
|---|---|---|
| DXGI | DirectX Graphics Infrastructure: the part of Windows between programs and the graphics drivers | — |
| factory | the counter where the other DXGI objects are asked for; here it hands out more than it creates (`EnumAdapters1`, not "Create") | one |
| adapter | whatever Windows treats as a graphics card | Intel, NVIDIA, and a software one |
| output | a screen attached to an adapter | the laptop panel |

- "Adapter" is the old PC name for a graphics card ("display adapter").
  It is not a cable converter and not a video encoder.
- GPU names the hardware; adapter is Windows' word for each entry in
  its list. They differ only for the software adapter, which is an
  adapter but not a GPU.
- The Intel chip holds a CPU and a small GPU on the same silicon
  (integrated graphics); the NVIDIA is a separate chip with its own
  memory (dedicated).

## Pointers, in the terms that worked

- A pointer is a variable whose content is an address. The `*` in the
  declaration is what makes it one.
- The model used: memory as numbered houses. `adapter` holds 6000
  (where the object is); `&adapter` is 108 (where the variable itself
  is); `hr` is unrelated to both — it is the success/failure number.
- `&x` appears once, when asking, and means "write the answer into this
  variable". `x->...` appears every other time and means "use the
  object this variable holds".
- The parallel from JavaScript:
  `const el = document.querySelector('#x')` — the element already
  exists, and `el` is a reference to it, not a copy. That is a pointer.
  Windows needs the `&` route only because the return value is already
  taken by `hr`.
- The object is reached by pointer (it belongs to Windows and is given
  back with `Release()`); information about it is copied into a struct
  we own (`desc`, in the next step).
- The name is literal, and the person reached this on their own: the
  variable does not contain the thing, it *points* to where the thing
  is. That is why it is called a pointer.
- Why `&adapter` and not `adapter`: a function receives a copy of a
  variable's content, never the variable (true in JavaScript too). The
  content is still "nothing" at that moment, so the function needs to
  know where our variable lives. We give 108 to receive 6000.
- `Release` here means "let go", not "launch a version" — another
  English word with two meanings, like "factory". It tells Windows we
  are done with the object; Windows counts users and destroys the
  object when the count reaches zero. It does not clear our variable,
  which keeps the old address — so `Release()` is always the last thing
  done with a pointer.

## What did not work, and the adjustment

- Lessons with several ideas and long code at once. Adjusted to one
  idea per step, each run on the Acer before the next.
- Questions that carried a hint containing the answer.
