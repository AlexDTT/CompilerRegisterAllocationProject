# Test Datasets

| File | Scenario |
|---|---|
| `inputs/basic/ranges/ranges1.txt` + `inputs/basic/registers/registers2.txt` | Same structure as the project description example with two webs for `i` and one for `sum`. |
| `inputs/basic/ranges/ranges2.txt` + `inputs/basic/registers/registers2.txt` | Web fusion example where `x` is built from multiple overlapping ranges. |
| `inputs/basic/ranges/ranges3.txt` + `inputs/basic/registers/registers2.txt` | Aggregated-web example where a merged web contains multiple internal fusion points. |
| `inputs/basic/ranges/ranges4.txt` + `inputs/basic/registers/registers1.txt` | Single-register chain where definition/last-use adjacency allows full reuse. |
| `inputs/basic/ranges/ranges5.txt` + `inputs/basic/registers/registers1.txt` | Another single-register chain with three independent variables. |
| `inputs/basic/ranges/ranges6.txt` + `inputs/basic/registers/registers3.txt` | Wider interference pattern that genuinely requires three registers. |
| `tests/input/spilling_triangle_*` | Clique of size 3 colored with 2 registers by spilling one web. |
| `tests/input/splitting_bridge_*` | Clique reduced to a 2-colorable graph after one split. |
| `tests/input/free_triangle_*` | Custom allocator using selective spilling on a 3-clique. |

## Test Strategy

The project uses two complementary layers:

- `tests/register_alloc_tests.cpp`: white-box unit tests for parsing, web construction,
  interference detection, output serialization, and all allocation variants.
- `tests/run_integration_tests.sh`: batch-mode end-to-end tests that compare generated
  allocations with deterministic expected outputs in `tests/output/expected`.
