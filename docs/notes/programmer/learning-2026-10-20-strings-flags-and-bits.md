# Note — what was learned on 2026-10-15 and 2026-10-20: text, switch, bit flags

- From: the `programmer` session working on Phase 1, step 1.
- Raw material for a `docs/LEARNING_LOG.md` entry; not a spec.

## A text does not fit in a variable

A number such as `hr` fits in its variable. A text does not: each
letter takes a place in memory, plus an end mark, so "Ana" takes four.
The variable holds only where the text starts:

    const char* vendorName = "Intel";

- `char*`: it holds an address, and letters are there. `const`: the
  letters cannot be changed through it; the variable can still be set
  to another text.
- The text was put in memory by the compiler, inside the `.exe`, not
  received from Windows, so there is nothing to `Release()`.
- A pointer is not about who made the thing: it exists because the
  thing is bigger than a variable, whose size is fixed by its type.
- `vendorName` is what is inside the variable (where the letters are);
  `&vendorName` is where the variable itself is. `&` is for handing a
  variable to someone else to fill, as with `EnumAdapters1`. Writing
  `&vendorName = "Intel"` was the person's first attempt.
- In `printf`, `%s` takes these texts; `%ls` is for Windows' wide text,
  such as `desc.Description`.

## switch

Same as in JavaScript, fall-through included: without `break`, the
following cases run without being tested. `default` is the case for
any other value, here `unknown`. Considered and not used: a map
(`std::map` exists in C++, but brings several new ideas at once).

## Binary, flags and `&` — taught from scratch on 2026-10-20

The first attempt (2026-10-15) stacked five ideas and was stopped.
The second went in four pieces:

1. **A software adapter** — a program doing a graphics card's work on
   the processor. The person's own link: Chrome's "use hardware
   acceleration" switch.
2. **Reading binary** — each place is worth double the one to its
   right (8, 4, 2, 1); add the places that hold a 1. Converting by
   hand is not needed here.
3. **Flags** — the person's feature flags, but many of them packed
   into one number, one place each. Their values double (1, 2, 4, 8)
   because a value like 3 would be two flags at once.
4. **`&`** — place by place, 1 only where both are 1. `desc.Flags`
   is the answer about one adapter; `DXGI_ADAPTER_FLAG_SOFTWARE` is a
   fixed constant naming the place (the person's word: the answer
   key). `desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE` is non-zero exactly
   when that flag is on — the C equivalent of `desc.flags.software` in
   JavaScript.

`==` would fail: an adapter both remote and software has flags `3`,
and `3 == 2` is false. The person asked why binary was needed at all
if the values are already known; that case is the answer.

A number used as a condition is false when 0 and true otherwise, as
with JavaScript's truthy and falsy.

`&` has two meanings in C++: before a variable, its address; between
two values, bitwise AND.
