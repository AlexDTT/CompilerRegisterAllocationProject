# Compiler Register Allocation Project

<img src="logo.png" alt="Compiler Register Allocation Project Logo" width="100">

## Algorithm Design (DA) - Spring 2026
**Course:** Algorithm Design (L.EIC016)  
**Project:** Programming Project II - Compiler Register Allocation Project

## About the Project
This project implements a compiler back-end support tool for **global register allocation**.
Given the live ranges of program variables, the application:

- merges compatible live ranges into **webs**;
- builds the corresponding **interference graph**;
- allocates webs to a bounded number of physical registers;
- falls back to **spilling** or **splitting** when required by the selected algorithm.

The tool includes four allocation modes:

| Mode | Summary | Main idea |
|---|---|---|
| `basic` | Pure graph coloring | Simplify-and-select coloring without user-authorized recovery actions. |
| `spilling, K` | Coloring with bounded spilling | Removes up to `K` webs from the graph and assigns them to memory. |
| `splitting, K` | Coloring with bounded splitting | Splits up to `K` webs into derived webs, rebuilds the graph, and retries coloring. |
| `free` | Custom heuristic | Uses a DSATUR-style ordering with selective spill fallback. |

## Group T9G2 Members
This project was developed by Group T9G2:
- **Alexandre Dinis Alves Teixeira** - [up202403579@up.pt](mailto:up202403579@up.pt)
- **Carlos Francisco de Sousa Ferreira Magalhães Diogo** - [up202404033@up.pt](mailto:up202404033@up.pt)
- **Rodrigo Martins Dias** - [up202404130@up.pt](mailto:up202404130@up.pt)

## Build

```bash
make
make test
make docs
make clean
```

The main executable is `./register_alloc`.

## Usage
The application supports both batch mode and an interactive terminal menu.

<div class="tabbed">

- <b class="tab-title">Batch Mode (Recommended)</b>
  ```bash
  ./register_alloc -b <ranges.txt> <config.txt> <allocation.txt>
  ```
  Example:
  ```bash
  ./register_alloc -b \
      inputs/basic/ranges/ranges1.txt \
      inputs/basic/registers/registers2.txt \
      allocation.txt
  ```

- <b class="tab-title">Interactive Mode</b>
  ```bash
  ./register_alloc
  ```
  The menu can load both input files, build webs, inspect the interference graph, run
  any allocation mode, and export a DOT graph for visualization.

</div>

## Input and Output Format

### Live ranges
Each line describes one live range:

```text
sum: 7+,8,9,10-
i: 1+,2,3,4,7
i: 7,8
i: 8,9-
```

Markers have the same meaning as in the project statement:
- `+` marks the definition that starts a live range.
- `-` marks the last use that ends a live range.
- unmarked points represent internal live points or merge/intersection points.

### Register/config file

```text
registers: 2
algorithm: spilling, 1
```

### Allocation output
The output contains:
- one line per web with its aggregated program points sorted in ascending order;
- the number of registers actually used;
- the mapping `rX: webY` or `M: webY`.

## System Architecture
The application pipeline is intentionally close to the problem formulation from the statement:

<div class="interactive_dotgraph">

\dotfile dox/annotated_architecture.dot "Compiler Register Allocation Project Pipeline"

</div>

## Example Interference Graphs

### Basic coloring example
<div class="interactive_dotgraph">

\dotfile dox/example_basic.dot "Basic allocation example"

</div>

### Spilling example
<div class="interactive_dotgraph">

\dotfile dox/example_spilling.dot "Spilling one web in a 3-clique"

</div>

### Splitting example
<div class="interactive_dotgraph">

\dotfile dox/example_splitting.dot "Splitting a web to reduce the chromatic requirement"

</div>

### Free heuristic example
<div class="interactive_dotgraph">

\dotfile dox/example_free.dot "Custom allocator with selective spill fallback"

</div>

## Complexity Summary

| Step | Complexity |
|---|---|
| Parse ranges file | `O(L * P)` |
| Parse config file | `O(L)` |
| Build webs | `O(V * R^2 * P)` |
| Build interference graph | `O(W^2 * P)` |
| Basic coloring | `O(W * (W + E))` |
| Spilling allocator | `O(K * W * (W + E))` |
| Splitting allocator | `O(K * (W^2 * P + W * (W + E)))` |
| Free allocator | `O(W^2 + E)` |

Where:
- `L` is the number of input lines,
- `P` is the average number of program points per range,
- `R` is the number of ranges per variable,
- `V` is the number of variables,
- `W` is the number of webs,
- `E` is the number of interference edges,
- `K` is the maximum number of allowed spill/split recovery actions.

## Test Coverage
The repository includes:

- six baseline datasets mirrored under `inputs/basic/`;
- white-box unit tests for parsing, graph construction, spilling, splitting, and output generation;
- deterministic integration tests that compare batch-mode outputs against expected files.

Run everything with:

```bash
make test
```

## Additional Documentation
For a deeper explanation of the data model, graph decisions, algorithmic rationale,
heuristics, and per-task complexity analysis, see
@ref extra_docs "Extra Documentation".
