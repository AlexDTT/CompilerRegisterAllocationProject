#set page(width: 16cm, height: 9cm, margin: 0cm, fill: rgb("#1C1D1F"))
#set text(font: "DejaVu Sans", size: 10pt, fill: rgb("#d2dbde"))
#set heading(numbering: none)

#let bg = rgb("#1C1D1F")
#let panel = rgb("#252628")
#let code = rgb("#282c34")
#let fg = rgb("#d2dbde")
#let muted = rgb("#859399")
#let orange = rgb("#ff8800")
#let orange-light = rgb("#ffa033")

#let pill(body) = box(
  inset: (x: 5pt, y: 2pt),
  radius: 3pt,
  fill: code,
  stroke: 0.4pt + rgb("#38393b"),
  text(size: 8pt, fill: orange-light)[#body],
)

#let panelbox(body) = rect(
  fill: panel,
  radius: 5pt,
  inset: 7pt,
  stroke: 0.4pt + rgb("#38393b"),
)[#body]

#let slide(title, body) = [
  #rect(width: 100%, height: 100%, fill: bg)[
    #place(top + right, dx: -0.38cm, dy: 0.28cm)[#image("logo.png", width: 0.42cm)]
    #place(top + left)[#rect(width: 0.08cm, height: 100%, fill: orange)]
    #pad(x: 0.68cm, y: 0.44cm)[
      #text(size: 15pt, weight: "bold", fill: fg)[#title]
      #v(0.10cm)
      #line(length: 1.4cm, stroke: 1.2pt + orange)
      #v(0.20cm)
      #body
    ]
  ]
]

// --- Slide 1: Title ---
#slide("Compiler Register Allocation", [
  #grid(columns: (1.1fr, 1.9fr), gutter: 0.5cm)[
    #align(center + horizon)[#image("logo.png", width: 2.0cm)]
  ][
    #text(size: 13pt, weight: "bold", fill: orange-light)[DA Programming Project II]
    #v(0.12cm)
    #text(size: 9pt, fill: muted)[Group T9G2  /  Spring 2026]
    #v(0.28cm)
    Alexandre Dinis Alves Teixeira \
    Carlos Francisco de Sousa Ferreira Magalhaes Diogo \
    Rodrigo Martins Dias
    #v(0.30cm)
    #pill[`make test`]  #pill[`make docs`]  #pill[`./register_alloc -b ...`]
  ]
])

// --- Slide 2: Pipeline ---
#pagebreak()
#slide("Pipeline", [
  #align(center)[#image("presentation_assets/pipeline_graph.svg", height: 2.35cm)]
  #v(0.08cm)
  #text(size: 7.8pt, fill: muted)[Course `Graph<int>` as primary representation. Web ids are vertex labels.]
])

// --- Slide 3: Recovery Modes ---
#pagebreak()
#slide("Spilling and Splitting", [
  #grid(columns: (1fr, 1fr), gutter: 0.4cm)[
    #panelbox[
      #text(weight: "bold", fill: orange-light, size: 9.5pt)[Bounded spilling]
      #v(0.06cm)
      #align(center)[#image("presentation_assets/spilling_triangle.svg", height: 3.8cm)]
    ]
  ][
    #panelbox[
      #text(weight: "bold", fill: orange-light, size: 9.5pt)[Bounded splitting]
      #v(0.06cm)
      #align(center)[#image("presentation_assets/splitting_bridge.svg", height: 3.8cm)]
    ]
  ]
  #v(0.08cm)
  #text(size: 7.8pt, fill: muted)[Splitting preserves original markers. `1+,2,5,6-` splits into `1+,2` and `5,6-` — no fabricated `2-` or `5+`. Metadata: `spills: N` / `splits: N`.]
])

// --- Slide 4: Free Algorithm ---
#pagebreak()
#slide("Custom Free Algorithm", [
  #grid(columns: (1fr, 1fr), gutter: 0.45cm)[
    #panelbox[
      #text(weight: "bold", fill: orange-light, size: 9.5pt)[Hybrid strategy]
      #v(0.08cm)
      - edgeless graphs: 1 register
      - complete graphs: K colored, rest spilled
      - bipartite graphs: BFS 2-coloring
      - general: DSatur greedy coloring
      - small/medium: branch-and-bound refines spills
    ]
  ][
    #align(center)[#image("presentation_assets/free_triangle.svg", height: 4.5cm)]
  ]
  #v(0.10cm)
  #rect(fill: code, radius: 4pt, inset: 6pt)[
    #text(size: 8pt)[Invariant: interfering colored webs never share a register. Memory only for uncolored webs.]
  ]
])

// --- Slide 5: Demo Commands ---
#pagebreak()
#slide("Demo and Testing", [
  #grid(columns: (1fr, 1fr), gutter: 0.45cm)[
    #panelbox[
      #text(weight: "bold", fill: orange-light, size: 9.5pt)[Inputs for the demo]
      #v(0.08cm)
      - `complex_allocation.txt` + `free4.txt`: dense allocator stress test
      - `splitting_showcase.txt` + `splitting2.txt`: split reduces K from 3 to 2
      - `spilling5.txt`: bounded spilling in a dense graph
    ]
  ][
    #rect(fill: code, radius: 5pt, inset: 7pt)[
      #text(font: "DejaVu Sans Mono", size: 7.5pt, fill: fg)[
        ./register_alloc -b \
        \u{0020}\u{0020}inputs/advanced/ranges/complex_allocation.txt \
        \u{0020}\u{0020}inputs/advanced/registers/free4.txt \
        \u{0020}\u{0020}alloc.txt
      ]
    ]
  ]
  #v(0.08cm)
  #text(size: 7.5pt, fill: muted)[`make test` — 76 unit + 9 integration (passes clean). DOT graphs written alongside allocation output.]
])

// --- Slide 6: Complexity ---
#pagebreak()
#slide("Complexities", [
  #let complexity-row(label, formula) = [
    #grid(columns: (1.5cm, 1fr), gutter: 0.2cm, align: horizon)[
      #text(size: 7.5pt, weight: "bold", fill: fg)[#label]
    ][
      #text(size: 8pt, fill: fg)[#formula]
    ]
    #v(0.04cm)
  ]
  
  #grid(columns: (1fr, 1fr), gutter: 0.4cm)[
    #panelbox[
      #text(weight: "bold", fill: orange-light, size: 9pt)[Input & graph building]
      #v(0.12cm)
      #complexity-row("Parse:", $O(L P)$)
      #complexity-row("Build webs:", $O(V R^2 P log P)$)
      #complexity-row("Interference:", $O(W^2 P + E W)$)
      #v(0.08cm)
    ]
  ][
    #panelbox[
      #text(weight: "bold", fill: orange-light, size: 9pt)[Coloring algorithms]
      #v(0.12cm)
      #complexity-row("Basic:", $O(W (W^2 + E))$)
      #complexity-row("Spilling:", $O((S+1) W (W^2 + E))$)
      #complexity-row("Splitting:", $O(S Q (W^2 P + E W + W(W^2+E)))$)
      #complexity-row("Free:", [$O(W^2 + E log W)$ (heuristic)])
    ]
  ]
  
  #v(0.10cm)
  #rect(fill: code, radius: 3pt, inset: 5pt)[
    #text(size: 7pt, fill: muted)[
      *Variables:* `W`=webs, `E`=edges, `P`=points, `R`=ranges/var, `S`=recovery bound, `Q`=split candidates, `L`=live ranges.
    ]
  ]
])
