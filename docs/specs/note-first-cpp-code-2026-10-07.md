# Samuel's first C++ code — 2026-10-07

Kept as a record, not as a lesson. This is the first C++ Samuel wrote
himself in this project: the loop that asks each graphics adapter for
its outputs (Phase 1, step 1, piece 3.1). Until this point he had read
and understood code line by line; this one he wrote from a table of
ingredients, using the adapter loop as a model.

In his own words, on the day: "estou muito feliz de finalmente estar me
dedicando à programação da forma que eu queria".

## What he was given

| | Adapter loop (already working) | Output loop (to write) |
|---|---|---|
| Who we ask | `factory` | `adapter` |
| Method | `EnumAdapters1(i, &adapter)` | `EnumOutputs(j, &output)` |
| Pointer type | `IDXGIAdapter1*` | `IDXGIOutput*` |
| Counter | `i` | `j` |
| End of list | `DXGI_ERROR_NOT_FOUND` | the same |

And the place to put it: inside the adapter loop, after the line that
prints the adapter's name.

## The code exactly as he wrote it

Unedited, including the surrounding lines he copied as context.

```cpp
for (UINT i = 0; ; i++) {
    IDXGIAdapter1* adapter = nullptr;
    hr = factory->EnumAdapters1(i, &adapter);
    // ... testes de erro ...

    DXGI_ADAPTER_DESC1 desc;
    hr = adapter->GetDesc1(&desc);
    // ... teste de erro ...

    printf("Adapter %u: %ls\n", i, desc.Description);

    // >>>>>>  O SEU CÓDIGO ENTRA AQUI  <<<<<<
    for(UINT j=0; ; j++){

        IDXGIOutput* output = nullptr
        hr = adapter->EnumOutputs(j, &output)

        if (hr == DXGI_ERROR_NOT_FOUND) {
            break;
        }

        if (FAILED(hr)) {
            // Talvez o hexadecial esteja errado, não sei
            printf("EnumOutputs(%u) failed: 0x%08lX\n", j, hr);
            output->Release();
            return 1;
        }
        
        printf("Output (%u) found.", j)
        output-Release()
        
    }
    adapter->Release();
}
```

His note with it: "fiquei na dúvida de implementar mais um for dentro
daqui, mas por você ter falado do j, imaginei que precisasse".

## What was right on the first try

- A loop nested inside the other, with its own counter `j`.
- The request: an empty pointer, `&output`, the answer in `hr`.
- Asking `adapter`, not `factory`.
- Testing `DXGI_ERROR_NOT_FOUND` before `FAILED`.
- Releasing the output at the end of every turn.
- The hexadecimal format — the one he was unsure about was correct.

## The mistakes

Syntax, none of them a reasoning error:

- Three statements without the final `;` (optional in JavaScript,
  required in C++).
- `output-Release()` instead of `output->Release();`.
- The output `printf` without `\n`.

One reasoning error, in the `if (FAILED(hr))` block:

- It called `output->Release()`, but at that point `EnumOutputs` had
  just failed, so the output was never received and the pointer was
  still `nullptr`. That call would have crashed the program.
- It did not release `adapter` and `factory`, both of which were held.

His question when this was pointed out — "mas no for com o i já tem
esse release, aqui também precisaria?" — led to the idea behind the
fix: `break` leaves only the loop it is in, while `return` leaves
`main` at once and skips every release further down. So each `return`
must give back everything held at that moment.

## The corrected block

```cpp
if (FAILED(hr)) {
    printf("EnumOutputs(%u) failed: 0x%08lX\n", j, hr);
    adapter->Release();
    factory->Release();
    return 1;
}
```

## Where it ended up

Committed as `02e0abb`, "feat(server-windows): list the outputs of each
adapter", in `server-windows/src/adapters_test.cpp`.
