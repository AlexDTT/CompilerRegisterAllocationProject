# Advanced Demo Inputs

`ranges/complex_allocation.txt` is a deliberately dense example for demos. It
contains fused webs (`i`, `flag`, `acc`), high-pressure overlap around the middle
program points, and late short-lived webs that show register reuse. Use it with
`free4.txt` or `spilling5.txt`.

`ranges/splitting_showcase.txt` is smaller on purpose: it isolates the corrected
web-splitting behavior so the demo can clearly show one split making the graph
2-colorable without fabricating new `+` or `-` markers at the split boundary.

Example commands:

```bash
./register_alloc -b inputs/advanced/ranges/complex_allocation.txt inputs/advanced/registers/free4.txt advanced_free.txt
./register_alloc -b inputs/advanced/ranges/complex_allocation.txt inputs/advanced/registers/spilling5.txt advanced_spilling.txt
./register_alloc -b inputs/advanced/ranges/splitting_showcase.txt inputs/advanced/registers/splitting2.txt advanced_splitting.txt
```

Each command writes the allocation text file and a matching colored `.dot` graph
with the same base name.
