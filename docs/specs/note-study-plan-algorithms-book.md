# Note — study plan: the algorithms book, done in C and C++

- Date: 2026-10-06
- From: a conversation in the `programmer` session during Phase 1,
  step 1. Not a spec for project code: a side plan the person asked to
  keep, so a later session can pick it up and guide the study.
- Book: "Entendendo Algoritmos" (Aditya Bhargava; original title
  "Grokking Algorithms"). The book's own examples are in Python.

## Why it complements this project

The book teaches how to think about a problem (Big O, data structures,
recursion, sorting, graphs). This project teaches how to talk to the
operating system (Windows APIs, the build, hardware). They meet at
pointers and manual memory handling — the exact point where the person
got stuck on 2026-10-06. Writing a linked list in C is the most direct
practice for reading `factory->Release()` and `&factory` here.

## The strategy agreed

C by default; C++ only when C "does not serve".

- **Stay in C** when the difficulty is the chapter's own subject
  (linked lists, recursion, quicksort). Struggling with pointers there
  is the learning.
- **Switch to C++** when the chapter uses a structure as a ready-made
  tool and its subject is something else (example: Dijkstra needs a
  hash table, but the chapter is about graphs).
- In C++, use only what was missing (`std::vector`,
  `std::unordered_map`, `std::queue`) and keep writing the rest the way
  it would be written in C.
- In the hash table chapter, build one in C at least once, even a
  simple one, before using the C++ one in later chapters.

## Considered and set aside

Doing every exercise in C, C++ and TypeScript. The algorithm is the
same in all three, so the second and third versions teach only language
differences and triple the time per exercise. If TypeScript is used at
all, it is as a quick sketch *before* the C version, when the algorithm
itself is unclear — to separate "I don't understand the algorithm" from
"I don't understand C". All three only for a few landmark exercises
(linked list, hash table), where the comparison is the point.

## Caveats

- Which chapters fall on each side of the C / C++ line was described
  from the assistant's memory of the book, not checked against the
  person's copy. Chapter order may differ by edition.
- C and C++ are different languages. Pointers carry over fully; memory
  handling changes name (`malloc`/`free` in C, `Release()` for the
  Windows objects in this project).

## The person's background

JavaScript/TypeScript professionally. C was their first language for
learning algorithms, never used professionally. This is their first
compiled language in practice.
