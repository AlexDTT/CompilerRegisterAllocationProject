#set page(width: 16cm, height: 9cm, margin: 0cm, fill: rgb("#1C1D1F"))
#set text(font: "DejaVu Sans", size: 10.5pt, fill: rgb("#d2dbde"))
#set heading(numbering: none)

#let bg = rgb("#1C1D1F")
#let panel = rgb("#252628")
#let code = rgb("#282c34")
#let fg = rgb("#d2dbde")
#let muted = rgb("#859399")
#let orange = rgb("#ff8800")
#let orange-light = rgb("#ffa033")

#let pill(body) = box(
  inset: (x: 6pt, y: 3pt),
  radius: 4pt,
  fill: code,
  stroke: 0.5pt + rgb("#38393b"),
  text(size: 8.5pt, fill: orange-light)[#body],
)

#let panelbox(body) = rect(
  fill: panel,
  radius: 6pt,
  inset: 8pt,
  stroke: 0.5pt + rgb("#38393b"),
)[#body]

#let slide(title, body) = [
  #rect(width: 100%, height: 100%, fill: bg)[
    #place(top + right, dx: -0.42cm, dy: 0.32cm)[#image("logo.png", width: 0.46cm)]
    #place(top + left)[#rect(width: 0.09cm, height: 100%, fill: orange)]
    #pad(x: 0.72cm, y: 0.48cm)[
      #text(size: 17pt, weight: "bold", fill: fg)[#title]
      #v(0.12cm)
      #line(length: 1.55cm, stroke: 1.3pt + orange)
      #v(0.25cm)
      #body
    ]
  ]
]

#slide("Compiler Register Allocation", [
  #grid(columns: (0.82fr, 2.2fr), gutter: 0.55cm)[
    #align(center + horizon)[#image("logo.png", width: 2.15cm)]
  ][
    #text(size: 14.5pt, weight: "bold", fill: orange-light)[DA Programming Project II]
    #v(0.16cm)
    #text(size: 9.5pt, fill: muted)[Group T9G2 - Spring 2026]
    #v(0.32cm)
    Alexandre Dinis Alves Teixeira \
    Carlos Francisco de Sousa Ferreira Magalhaes Diogo \
    Rodrigo Martins Dias
    #v(0.36cm)
    #pill[`make test`] #h(0.12cm) #pill[`make docs`] #h(0.12cm) #pill[`./register_alloc -b ...`]
  ]
])

#pagebreak()
#slide("Pipeline", [
  #grid(columns: (1fr, 1fr), gutter: 0.5cm)[
    #panelbox[
      #text(weight: "bold", fill: orange-light)[Core flow]
      #v(0.12cm)
      - parse ranges + config
      - fuse ranges into webs
      - build the interference graph
      - color with at most `K` registers
      - export allocation text and colored DOT
    ]
    #v(0.22cm)
    #text(size: 8.8pt, fill: muted)[Doxygen output in `docs/html` documents the graph model, algorithms, and complexity analysis.]
  ][
    #align(center)[#image("presentation_assets/splitting_example.svg", height: 4.7cm)]
  ]
])

#pagebreak()
#slide("Spilling and Splitting", [
  #grid(columns: (1fr, 1fr), gutter: 0.35cm)[
    #panelbox[
      #text(weight: "bold", fill: orange-light)[Bounded spilling]
      #v(0.08cm)
      #align(center)[#image("presentation_assets/spilling_triangle.svg", width: 5.35cm)]
    ]
  ][
    #panelbox[
      #text(weight: "bold", fill: orange-light)[Bounded splitting]
      #v(0.08cm)
      #align(center)[#image("presentation_assets/splitting_bridge.svg", width: 5.35cm)]
    ]
  ]
  #v(0.12cm)
  #text(size: 8.2pt, fill: muted)[Metadata remains parseable: `spills: N`, `spill: webX`, `splits: N`, `split: webX -> webY,webZ`.]
])

#pagebreak()
#slide("Custom Free Algorithm", [
  #grid(columns: (1.1fr, 0.9fr), gutter: 0.45cm)[
    #panelbox[
      #text(weight: "bold", fill: orange-light)[Pressure-aware choice]
      #v(0.12cm)
      - count distinct neighbor register colors
      - choose the web with the highest count
      - break ties by highest graph degree
      - assign the lowest compatible register
      - use memory only when all registers conflict
    ]
    #v(0.22cm)
    #rect(fill: code, radius: 5pt, inset: 7pt)[
      #text(size: 8.7pt)[Invariant: interfering colored webs never share a register.]
    ]
  ][
    #align(center)[#image("presentation_assets/free_triangle.svg", height: 5.0cm)]
  ]
])

#pagebreak()
#slide("Demo Inputs and Commands", [
  #grid(columns: (1fr, 1fr), gutter: 0.42cm)[
    #panelbox[
      #text(weight: "bold", fill: orange-light)[Advanced examples]
      #v(0.12cm)
      - `complex_allocation.txt`
      - `free4.txt`
      - `spilling5.txt`
      - `splitting_showcase.txt`
      - `splitting2.txt`
    ]
    #v(0.18cm)
    #text(size: 8.5pt, fill: muted)[The dense input shows fused webs, high pressure, register reuse, and memory assignments.]
  ][
    #rect(fill: code, radius: 6pt, inset: 8pt)[
      #text(font: "DejaVu Sans Mono", size: 7.9pt, fill: fg)[
        make test \
        make docs \
        ./register_alloc -b \
        inputs/advanced/ranges/complex_allocation.txt \
        inputs/advanced/registers/free4.txt \
        advanced_free.txt
      ]
    ]
  ]
])

#pagebreak()
#slide("Complexities", [
  #set text(size: 9.2pt)
  #grid(columns: (1fr, 1fr), gutter: 0.35cm)[
    #panelbox[
      Build webs: $O(V dot R^2 dot P log P)$ \
      Build graph: $O(W^2 dot P + E dot W)$ \
      Basic: $O(W dot (W^2 + E))$
    ]
  ][
    #panelbox[
      Spilling: $O((S + 1) dot W dot (W^2 + E))$ \
      Splitting: $O((S + 1) dot (W^2 dot P + E dot W + W dot (W^2 + E)))$ \
      Free: $O(W dot (W^2 + E))$
    ]
  ]
  #v(0.25cm)
  #text(size: 8.3pt, fill: muted)[`W` webs, `E` directed adjacency entries, `P` program points, `R` ranges per variable, `S` recovery bound. Bounds include the vector-backed `Graph<int>::findVertex` cost.]
])
