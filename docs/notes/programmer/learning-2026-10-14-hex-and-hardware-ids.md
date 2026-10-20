# Note — what was learned on 2026-10-14: hexadecimal and hardware IDs

- From: the `programmer` session working on Phase 1, step 1.
- Raw material for a `docs/LEARNING_LOG.md` entry; not a spec.
- The person found this part especially interesting: the program's
  numbers could be checked against Windows itself, outside our code.

## Hexadecimal is a way of writing, not a different number

A number is a quantity. Decimal and hexadecimal are two ways of
writing the same quantity: 255 and `FF`, 32902 and `8086`.

`0x` is a label, not a conversion. The person's own analogy: like the
currency sign before a price. "5" reads differently in reais and in
dollars; `10` is ten and `0x10` is sixteen. The sign only says how to
read the digits.

The conversion is done by `printf`'s marker, not by the label:

    printf("0x%04X\n", n);

    %   a marker starts here: a value goes in this place
    0   pad with zeros on the left (only zeros or, without it, spaces)
    4   at least 4 digits; a longer number is printed whole
    X   write it in hexadecimal, letters in upper case (x: lower case)

`0x` before the `%` is plain text. In the error lines, `0x%08lX` adds
`l` because `HRESULT` is a `long`; the IDs are `UINT`, so they take none.

| Line (`n = 255`) | Printed |
|---|---|
| `printf("%u\n", n);` | `255` |
| `printf("0x%X\n", n);` | `0xFF` |
| `printf("0x%04X\n", n);` | `0x00FF` |
| `printf("%4X\n", n);` | `  FF` |

## Checking the program against Windows

Device Manager → display adapter → Properties → Details → Hardware Ids
shows each card's IDs. They match what `adapters_test` printed:

| Card | Device Manager | `adapters_test` |
|---|---|---|
| Intel | `VEN_8086&DEV_5916` | `vendor: 0x8086, device: 0x5916` |
| NVIDIA | `VEN_10DE&DEV_134D` | `vendor: 0x10DE, device: 0x134D` |

The Microsoft Basic Render Driver printed `vendor: 0x1414` (Microsoft)
and is not in Device Manager's display adapters: it is not hardware.

The four lines Device Manager shows for each card are not cores or
GPUs. They describe the same card, from most specific to most generic;
Windows walks down them to choose a driver. The Intel's first line:

| Piece | Meaning |
|---|---|
| `VEN_8086` | chip maker: Intel |
| `DEV_5916` | chip model |
| `SUBSYS_11931025` | that chip as built into this laptop; `1025` believed to be Acer (unverified) |
| `REV_02` | chip revision |
| `CC_0300` | device class: what kind of device it is |

The class codes differ: Intel `CC_0300`, NVIDIA `CC_0302`. From memory,
**unverified**: `0300` is an ordinary display controller, `0302` a "3D
controller" with no display output of its own. If confirmed, the card
itself says what DXGI showed in step 1: no outputs under the NVIDIA.

## Earlier the same day: keeping a value after a loop

From the summary line, worked through in JavaScript first:

- A variable declared inside `{ }` ends at the closing `}`; to use
  something found inside a loop afterwards, declare it before the loop.
- Writing the type again inside (`let found = i;`, `UINT found = i;`)
  creates a second variable with the same name. No error; the outer one
  stays empty.
- JavaScript has `undefined` ("declared, no value"); C++ has no such
  thing: a variable with no value holds whatever was in that memory.
  So C++ code picks its own "not found" value.
- `0` could not mean "not found", because adapter 0 exists — and holds
  the desktop. `-1` works, like JavaScript's `indexOf`, but `UINT` is
  unsigned and cannot hold it, so the variable is an `int`.
- A test on the Acer would not have caught the first version, which
  stored any output, attached to the desktop or not: the Acer has only
  one. Only reading the code caught it.
