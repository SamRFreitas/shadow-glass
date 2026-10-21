# Spikes, and a pointer that outlives its object

- Date: 2026-10-21, Phase 1 step 2, before piece 1.

## Spike vs test

In the industry, "test" usually means an automated test: code that
checks other code and reports pass or fail. A small program written to
answer one question about the system is usually called a spike, or an
exploratory program. `adapters_test` is the second kind.

## Copying a COM pointer does not add an owner

    IDXGIAdapter1* intel = nullptr;
    // inside the loop:
    intel = adapter;     // copies the address, not the object
    adapter->Release();  // drops the only reference
    // after the loop:
    intel->GetDesc1(&desc);  // undefined behavior

The person's first reading, from JavaScript: both variables point to
the same object, so it works. In JS that holds, because the garbage
collector sees two references. In COM, copying a pointer does not touch
the reference count: the object still thinks it has one owner. After
`Release()` it may be destroyed, and `intel` still holds its old
address: a dangling pointer. Using it can crash, return garbage, or
work by luck.

Rule: after your `Release()`, that object is not yours to use, whatever
other variable holds the same address.
