# VectorDB

A vector database built from scratch in C++ with the long-term goal of efficiently searching **1 billion vectors in under 100 ms**.

## Approach

We are building the database incrementally, starting with brute-force vector search and improving the architecture only when the current implementation reaches its limits.
Every approach is benchmarked so we can measure the improvement and understand which bottleneck the new architecture solves.

---

# Approach 1 — Brute Force + `unordered_map<string, vector<float>>`

### Architecture

```text
unordered_map<string, vector<float>>
```

Every query scans every stored vector and calculates cosine similarity.

### Benchmark

**Dimension = 3, K = 2**

|    Vectors | Search Time |
| ---------: | ----------: |
|      1,000 |    0.806 ms |
|     10,000 |    5.625 ms |
|    100,000 |   18.919 ms |
| 10,000,000 | 2,072.76 ms |

### Result

**10M vectors → 2.07 seconds**

The brute-force approach scales linearly with the number of vectors.

---

# Approach 2 — Separate ID Storage from Vector Storage

### Architecture

```text
unordered_map<string, int>  → ID → vector index
vector<string>              → IDs
vector<vector<float>>       → vectors
```

The search no longer traverses the hash map. It scans the vector storage directly.

### Benchmark

**Dimension = 3**

|    Vectors | Search Time |
| ---------: | ----------: |
|        100 |    0.025 ms |
|      1,000 |    0.339 ms |
|     10,000 |    1.201 ms |
|    100,000 |    5.201 ms |
|  1,000,000 |   63.672 ms |
| 10,000,000 |   550.84 ms |

### Improvement

At 10M vectors:

```text
Approach 1 → 2072.76 ms
Approach 2 → 550.84 ms
```

Approximately **3.76× faster**.

> Note: the benchmark conditions were not perfectly identical (`K` differed), so this is an approximate comparison.

### Why?

The vector search path no longer walks through the hash-map structure and its per-entry storage overhead.

---

# Approach 3 — Contiguous 1D Vector Storage

### Architecture

We replaced:

```cpp
vector<vector<float>> embeddings;
```

with:

```cpp
vector<float> embeddings;
```

All vector values are stored in one contiguous block of memory.

For dimension `3`:

```text
[v0_0, v0_1, v0_2,
 v1_0, v1_1, v1_2,
 v2_0, v2_1, v2_2, ...]
```

The string IDs are still stored separately.

### Benchmark — Dimension = 3, K = 1

|    Vectors |    Search Time |
| ---------: | -------------: |
|        100 |       0.031 ms |
|      1,000 |       0.265 ms |
|     10,000 |       0.940 ms |
|    100,000 |       4.172 ms |
|  1,000,000 |      34.355 ms |
| 10,000,000 | **356.031 ms** |

### Benchmark — Dimension = 10, K = 1

|    Vectors |    Search Time |
| ---------: | -------------: |
|        100 |       0.069 ms |
|      1,000 |       0.281 ms |
|     10,000 |       2.253 ms |
|    100,000 |       ~9.09 ms |
|  1,000,000 |      88.481 ms |
| 10,000,000 | **885.041 ms** |

The 100K value is the average of two runs:

```text
9.176 ms
8.996 ms
→ 9.086 ms
```

### K = 20 test

For 10M vectors, dimension = 3:

```text
349.294 ms
351.578 ms
```

Average:

```text
350.436 ms
```

The small difference between K = 1 and K = 20 shows that, at this scale, scanning all vectors dominates the search cost.

### Improvement

Compared with Approach 2:

```text
Approach 2 → 550.84 ms
Approach 3 → 356.031 ms
```

Approximately **1.55× faster** under the measured conditions.

Compared with the original Approach 1:

```text
Approach 1 → 2072.76 ms
Approach 3 → 356.031 ms
```

Approximately **5.82× faster** under the measured conditions.

> These comparisons are directional rather than perfectly controlled because the K values differed between some experiments.

---

# Current Benchmark Summary

```text
                         10M vectors
                              │
        ┌─────────────────────┼─────────────────────┐
        ▼                     ▼                     ▼
   Approach 1            Approach 2            Approach 3
  Hash map storage       Separate storage      Contiguous storage

   2072.76 ms              550.84 ms              356.03 ms
        │                     │                     │
        └─────────────────────┴─────────────────────┘

                     Still brute force
```

We have improved the implementation substantially without changing the search algorithm itself.

---

# Current Limitation

Even after improving memory layout:

```text
10M vectors → ~356 ms
```

The database still compares the query against **every vector**.

The fundamental problem remains:

```text
Query
  ↓
Vector 1
Vector 2
Vector 3
...
Vector 10,000,000
```

This approach cannot scale to:

```text
1,000,000,000 vectors
```

while maintaining a sub-100 ms target.

---

# Next Approach

The next stage will investigate **SIMD/vectorization and low-level computation efficiency**.

The goal is to make the brute-force computation itself faster before introducing approximate nearest-neighbor indexing.

The same process continues:

```text
New approach
    ↓
Benchmark
    ↓
Compare with previous approach
    ↓
Measure improvement
    ↓
Understand the bottleneck
```
