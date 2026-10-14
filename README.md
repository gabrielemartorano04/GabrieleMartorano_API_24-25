# 🚛 MovHex — Algorithms and Data Structures Final Project (A.Y. 2024–2025)

Final project developed for the **Algorithms and Data Structures** module of the **Algorithms and Principles of Computer Science** (*Algoritmi e Principi dell'Informatica*) course at **Politecnico di Milano**.

* **University:** Politecnico di Milano — Department of Electronics, Information and Bioengineering (DEIB)
* **Professor:** Prof. Matteo Pradella
* **Author:** Gabriele Martorano
* **Final Grade:** 27 / 30
* **Programming Language:** C (C11 / gnu11 standard, C Standard Library only, single-threaded)

---

## 📋 Problem Description

**MovHex** is a logistics and trucking company managing a fleet of vehicles across a wide geographic region. The goal of the program is to compute minimum-cost routes for moving vehicles across a map modeled as a rectangular grid of hexagonal tiles.

### Map Characteristics
* **Coordinates:** Each hexagon is uniquely identified by its column and row indices `(x, y)`, numbered starting from `0` from left to right and from bottom to top. Hexagon `(0, 1)` is located to the top-right of hexagon `(0, 0)`.
* **Ground Adjacency:** Every non-border hexagon is connected to exactly 6 adjacent hexagonal tiles.
* **Ground Exit Costs:** Each hexagon has an associated integer cost in the range `[0, 100]` representing the cost of leaving that hexagon via ground toward an adjacent tile. A cost of `0` means the hexagon cannot be exited (impassable), although it can still be reached as the final destination of a route.
* **Air Routes:** Pairs of hexagons can be connected by directed (one-way) air routes. Each hexagon can have at most 5 outgoing air routes, each with its own traversal cost in the range `[0, 100]`.

---

## ⚙️ Commands Specification

The program reads a sequence of commands from `stdin` and prints the corresponding outputs to `stdout`:

1. **`init <cols> <rows>`**
   * Initializes (or re-initializes) a map of size `<cols> × <rows>`, discarding any previous map data.
   * Sets the ground exit cost of every hexagon to `1` with no active air routes, and prints `OK`.

2. **`change_cost <x> <y> <v> <radius>`**
   * Updates the ground exit costs of all hexagons (as well as the traversal costs of all their outgoing air routes) located at a hexagonal distance strictly less than `<radius>` from `(<x>, <y>)`, where `<radius> > 0` and `<v>` is an integer in `[-10, 10]`.
   * Given the hexagonal distance `d = HexDist((x_e, y_e), (x, y))`, the cost of each affected hexagon `(x_e, y_e)` and its outgoing air routes is updated according to:
        $$\text{cost}(x_e, y_e) = \text{cost}(x_e, y_e) + \left\lfloor v \times \max\left(0, \frac{\text{radius} - \text{HexDist}((x_e, y_e), (x, y))}{\text{radius}}\right) \right\rfloor$$
   * Updated costs are clamped to the interval `[0, 100]`.
   * Prints `OK` on success, or `KO` if the coordinates are out of bounds, `radius <= 0`, or `v` is out of range.

3. **`toggle_air_route <x1> <y1> <x2> <y2>`**
   * Adds (if absent) or removes (if already present) a directed air route from `(<x1>, <y1>)` to `(<x2>, <y2>)`.
   * When adding a new air route, its initial traversal cost is computed as the floored integer average of all currently active outgoing air routes from `(<x1>, <y1>)` and the ground exit cost of `(<x1>, <y1>)` itself (clamped to `[0, 100]`):
     $$\text{cost} = \left\lfloor \frac{\sum(\text{outgoing air route costs}) + \text{cost}_{(x_1, y_1)}}{\vert{}\text{outgoing air routes}\vert{} + 1} \right\rfloor$$
   * Prints `OK` on success, or `KO` if either coordinate pair is invalid or if `(<x1>, <y1>)` already has 5 outgoing air routes.

4. **`travel_cost <xp> <yp> <xd> <yd>`**
   * Computes and prints the minimum total cost to reach destination hexagon `(<xd>, <yd>)` starting from `(<xp>, <yp>)` using a combination of ground movements and air routes.
   * The exit cost of the destination hexagon is ignored. When taking an air route, the ground exit cost of the starting hexagon of that air route is ignored.
   * Prints `0` if the starting and destination hexagons coincide, or `-1` if the coordinates are invalid or the destination is unreachable.

---

## 🧠 Design Choices & Algorithm Architecture

The implementation is written in a single C source file and is designed to minimize dynamic memory allocation overhead and shortest-path query times:

* **Grid Representation (`HexMap` & `Hexagon`):** The map is stored as a dynamically allocated 2D grid where each `Hexagon` struct contains its `ground_cost`, `air_route_count`, and a fixed-size array `AirRoute air_routes[5]` to maximize cache locality. Hexagonal neighbor offsets are handled via constant lookup tables (`dx_even`/`dy_even` and `dx_odd`/`dy_odd`) selected based on row parity (`y % 2`).
* **Pre-allocated Search Buffers:** All auxiliary data structures (`dist_flat`, `bfs_queue`, `bfs_visited`, `bucket_head`, `bucket_next`) are allocated once during `init`, avoiding repeated `malloc`/`free` calls during `change_cost` and `travel_cost` executions.
* **BFS Cost Updates (`change_cost`):** Since unweighted hexagonal distance corresponds to the minimum number of steps on the hex grid, `change_cost` precomputes the cost delta for each distance layer `d < radius` in `delta_array` and performs a bounded Breadth-First Search (BFS) up to `<radius>`.
* **Dial's Algorithm / Bucket Queue (`travel_cost`):** Because every ground step or air route traversal has a non-negative integer cost bounded by `MAX_EDGE_COST = 100`, shortest-path queries are solved using **Dial's Algorithm** (a circular bucket priority queue of size `B = 101` implemented via intrusive array-based linked lists `bucket_head` and `bucket_next`). This eliminates binary heap logarithmic overhead and provides fast minimum extraction.

---

## 📁 Repository Structure

```text
.
├── MovHex.c       # C source code implementation
├── tests/         # Folder containing public test cases (inputs and expected outputs)
└── README.md      # Project documentation
```

---

## 🚀 Compilation and Setup

### Prerequisites
To compile and test the project on a Linux environment (Debian/Ubuntu or WSL), install `gcc` and the standard profiling/debugging tools:
```bash
sudo apt install build-essential gdb valgrind kcachegrind massif-visualizer
```

### Compilation
Compile the source code using the exact flags adopted by the official Polimi evaluation platform (`-Wall -Werror -std=gnu11 -O2 -lm`):
```bash
gcc -Wall -Werror -std=gnu11 -O2 "MovHex.c" -o movhex -lm
```

Alternatively, if you rename the source file to `main.c`, you can create a `Makefile` with:
```makefile
CFLAGS += -Wall -Werror -std=gnu11 -O2
LDFLAGS += -lm
```
and compile by running `make main`.

---

## 🧪 How to Run and Test

The program reads commands from `stdin` and writes responses to `stdout`.

### 1. Run a Single Test Case
Pass an input file from the `tests/` folder and redirect the output to a file:
```bash
./movhex < tests/input_1.txt > program_output.txt
```

Compare your output against the expected output using `diff` (if the files match, `diff` produces no output):
```bash
diff tests/output_1.txt program_output.txt
```

### 2. Quick Interactive Test
You can run the example from the official specification directly in the terminal:
```bash
./movhex << 'EOF'
init 100 100
change_cost 10 20 -10 5
change_cost 30 95 10 1
travel_cost 0 0 20 0
travel_cost 30 95 30 97
travel_cost 10 20 11 20
toggle_air_route 0 0 20 0
travel_cost 0 0 20 0
toggle_air_route 10 20 10 22
travel_cost 10 20 10 22
toggle_air_route 0 0 20 0
travel_cost 0 0 20 0
change_cost 200 20 -10 5
toggle_air_route 200 20 -10 5
travel_cost 200 20 11 20
init 20 20
change_cost 10 20 -10 5
EOF
```

Expected output:
```text
OK
OK
OK
20
12
-1
OK
1
OK
-1
OK
20
KO
KO
-1
OK
KO
```

---

## 🔍 Debugging and Performance Profiling

You can verify memory safety and profile runtime performance using the tools recommended during the course:

* **AddressSanitizer (ASAN)** — detect out-of-bounds memory accesses and memory corruption:
  ```bash
  gcc -Wall -Werror -std=gnu11 -O2 -g3 -fsanitize=address "MovHex.c" -o movhex_asan -lm
  ./movhex_asan < tests/input_1.txt > /dev/null
  ```

* **Valgrind Memcheck** — check for memory leaks, use-after-free, and uninitialized values (compile with `-g3` and **without** `-fsanitize=address`):
  ```bash
  gcc -Wall -Werror -std=gnu11 -O2 -g3 "MovHex.c" -o movhex_debug -lm
  valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./movhex_debug < tests/input_1.txt > /dev/null
  ```

* **Valgrind Callgrind** — profile CPU instruction counts and execution bottlenecks:
  ```bash
  valgrind --tool=callgrind ./movhex_debug < tests/input_1.txt > /dev/null
  kcachegrind callgrind.out.*
  ```

* **Valgrind Massif** — profile dynamic heap memory usage over time:
  ```bash
  valgrind --tool=massif ./movhex_debug < tests/input_1.txt > /dev/null
  massif-visualizer massif.out.*
  ```
