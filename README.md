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
- allocates webs to a bounded number of physical registers (using graph coloring);
- applies **spilling** or **splitting** when selected by the configuration file.

The tool includes four allocation modes:

| Mode | Summary | Main idea |
|---|---|---|
| `basic` | Pure graph coloring | Simplify-and-select coloring. It's the greedy algorithm provided by the teacher. |
| `spilling, K` | Coloring with bounded spilling | Removes up to `K` webs from the graph and assigns them to memory. |
| `splitting, K` | Coloring with bounded splitting | Splits up to `K` webs into derived webs, rebuilds the graph, and retries coloring. |
| `free` | Custom allocator | Uses graph-class fast paths, DSatur ordering, and bounded branch-and-bound to reduce spills. |

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

Markers use the live-range notation defined for the project:
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
- for alternative algorithms, a processing-friendly metadata section such as
  `spills: 1` / `spill: web0` or `splits: 1` / `split: web0 -> web0,web3`.

Batch mode also writes a colored DOT visualization next to the allocation file
by replacing the output extension with `.dot`. For example, `allocation.txt`
produces `allocation.dot`. Nodes are colored by register, memory-assigned webs
are gray boxes, and split-derived webs receive highlighted borders.

Splitting preserves the original live-range markers exactly. If a range such as
`1+,2,3,4-` is split into `1+,2` and `3,4-`, the second derived web is not forced
to become `3+,4-`; the split boundary is not a new definition or last use.

## System Architecture
The application pipeline follows the live-range to web to interference-graph workflow:

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

### Non-interference adjacency example
<div class="interactive_dotgraph">

\dotfile dox/example_noninterference_chain.dot "Definition at last-use line allows register reuse (red means no interference, the dashed edges are not actual interferences)"

</div>

### Web fusion example
<div class="interactive_dotgraph">

\dotfile dox/example_web_fusion.dot "Transitive fusion of overlapping live ranges"

</div>

## Complexity Summary

| Step | Complexity |
|---|---|
| Parse ranges file | `O(L * P)` |
| Parse config file | `O(L)` |
| Build webs | `O(V * R^2 * P log P)` |
| Build interference graph | `O(W^2 * P + E * W)` |
| Basic coloring | `O(W * (W^2 + E))` |
| Spilling allocator | `O((S + 1) * W * (W^2 + E))` |
| Splitting allocator | `O(S * Q * (W^2 * P + E * W + W * (W^2 + E)))` |
| Free allocator | `O(W^2 + E log W)` heuristic; guarded exact pass `O((K + 1)^W * (W + E))` |

Where:
- `L` is the number of input lines,
- `P` is the maximum number of program points considered in a parse, range, or web-pair comparison,
- `R` is the number of ranges per variable,
- `V` is the number of variables,
- `W` is the number of webs,
- `E` is the number of directed adjacency entries in the interference graph,
- `S` is the configured maximum number of spill/split recovery actions.
- `Q` is the number of candidate split positions evaluated in one split iteration.

The coloring bounds include the current vector-backed course `Graph<int>` implementation,
where `findVertex` is `O(W)`. Replacing the vertex store with an indexed map would reduce
several lookup-driven factors, but the project intentionally keeps the provided graph as
the primary representation.

The `free` allocator only runs the exponential branch-and-bound refinement below a
fixed small/medium graph-size threshold. Larger inputs keep the polynomial DSatur
result, which is more appropriate for an interactive demo tool.

## Project Requirements Coverage

| Requirement | Status |
|---|---|
| T1.1 command-line menu and batch mode | Implemented through `RegisterAllocApp` and `./register_alloc -b`. |
| T1.2 input parsing and graph-based data structures | Implemented through `FileParser`, `InterferenceGraph`, and the course `Graph<int>`. |
| T1.3 Doxygen documentation and complexity analysis | Source-level Doxygen plus this README and `dox/extra_documentation.dox`. |
| T2.1 basic allocation | Implemented. |
| T2.2 bounded spilling | Implemented. |
| T2.3 bounded splitting | Implemented. |
| T2.4 custom allocation | Implemented with graph-class fast paths, DSatur ordering, and bounded exact spill minimization. |
| T3.1 demo support | Interactive menu plus `Presentation.pdf`, graph assets, and editable `Presentation.typ` source. |
| Testing | Unit and integration tests are available through `make test`. |

## Test Coverage
The repository includes:

- six baseline datasets mirrored under `inputs/basic/`;
- advanced demo inputs under `inputs/advanced/`, including a dense custom/free
  case, a bounded-spilling case, and a focused splitting showcase;
- white-box unit tests for parsing, graph construction, spilling, marker-preserving splitting, free-mode coloring, and output generation;
- deterministic integration tests that compare batch-mode outputs against expected files.

Run everything with:

```bash
make test
```

## Additional Documentation
For a deeper explanation of the data model, graph decisions, algorithmic rationale,
heuristics, and per-task complexity analysis, see
@ref extra_docs "Extra Documentation".
