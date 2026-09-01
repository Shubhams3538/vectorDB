VectorDB

A vector database built from scratch in C++ with the long-term goal of efficiently searching 1 billion vectors in under 100 ms.

Approach

We are building the database incrementally, starting with brute-force vector search and improving the architecture only when the current implementation reaches its limits.
Every stage is benchmarked so we can identify the bottleneck and understand why the next optimization is needed.

Stage 1 — Brute-Force Search

The current implementation:

Stores vectors in memory using unordered_map<string, vector<float>>

Uses cosine similarity

Compares the query against every stored vector

Maintains the top-K results using a min-heap

Uses C++17

Uses CMake for building the project

Project Structure

vectorDB/
├── CMakeLists.txt
├── README.md
├── .gitignore
├── vector_db.h
├── vector_db.cpp
├── main.cpp
├── benchmark.cpp
└── build/ # generated locally, ignored by Git

Build

Configure the project:

cmake -S . -B build

Build:

cmake --build build

Run the database:

./build/app

Run the benchmark:

./build/benchmark

Benchmark — Stage 1

Configuration

Vector dimension: 3

Similarity metric: Cosine Similarity

Search strategy: Brute Force

Top-K: 2

Number of Vectors

Search Time

1,000

0.806 ms

10,000

5.625 ms

100,000

18.919 ms

10,000,000

2,072.76 ms

Observation

At 10 million vectors, brute-force search takes approximately 2.07 seconds for a single query.

A 100 million vector experiment was also attempted. The current unordered_map<string, vector<float>> representation became extremely memory-heavy, reaching roughly 9 GB+ of process memory during the experiment, so the run was stopped after several minutes.

The raw vector data for 100 million 3-dimensional float vectors is only about 1.2 GB, highlighting the overhead introduced by the current storage representation.

Next Step

The next stage is to investigate memory layout and locality by replacing the current hash-map-based representation with more contiguous storage and benchmarking the difference.

The eventual roadmap will move from brute force toward SIMD, ANN indexing, quantization, persistence, sharding, and eventually billion-scale search.
