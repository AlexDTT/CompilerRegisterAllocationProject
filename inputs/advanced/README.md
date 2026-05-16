# Advanced Demo Inputs

`ranges/complex_allocation.txt` is a deliberately dense example for demos. It
contains fused webs (`i`, `flag`, `acc`), high-pressure overlap around the middle
program points, and late short-lived webs that show register reuse. The default
demo pairing is `free4.txt`, which uses the normal free allocator and leaves
some webs spilled. Use `free_split4.txt` to show the same input with recovery
splitting enabled.

`ranges/splitting_showcase.txt` is smaller on purpose: it isolates the corrected
web-splitting behavior so the demo can clearly show one split making the graph
2-colorable without fabricating new `+` or `-` markers at the split boundary.

`ranges/free_split_spill_showcase.txt` demonstrates free-split recovery: the early
component improves after one split, while an independent 3-clique still forces
one spill with two registers.

Example commands:

```bash
./register_alloc -b inputs/advanced/ranges/complex_allocation.txt inputs/advanced/registers/free4.txt advanced_free.txt
./register_alloc -b inputs/advanced/ranges/complex_allocation.txt inputs/advanced/registers/free_split4.txt advanced_free_split.txt
./register_alloc -b inputs/advanced/ranges/complex_allocation.txt inputs/advanced/registers/spilling5.txt advanced_spilling.txt
./register_alloc -b inputs/advanced/ranges/splitting_showcase.txt inputs/advanced/registers/splitting2.txt advanced_splitting.txt
./register_alloc -b inputs/advanced/ranges/free_split_spill_showcase.txt inputs/advanced/registers/free_split2_recovery.txt advanced_free_recovery.txt
```

Each command writes the allocation text file and a matching colored `.dot` graph
with the same base name.
