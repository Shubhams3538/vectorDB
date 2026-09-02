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

# Approach 4 — Precompute Query Norm Once

### Change

Cosine similarity divides the dot product by the lengths of the query vector and stored vector.

The query does not change during one search, so its length is now calculated once before scanning the database:

```text
query_norm = sqrt(query[0]^2 + query[1]^2 + ... + query[D - 1]^2)
```

That single value is reused for every stored vector instead of being calculated again inside the scan loop.

### Benchmark — Dimension = 3, K = 1

|    Vectors | Search Time |
| ---------: | ----------: |
|        100 |    0.017 ms |
|      1,000 |    0.090 ms |
|     10,000 |    0.639 ms |
|    100,000 |    3.497 ms |
|  1,000,000 |   24.643 ms |
| 10,000,000 | **254.590 ms** |

### Improvement

At 10M vectors:

```text
Approach 3 -> 356.031 ms
Approach 4 -> 254.590 ms
```

This is approximately **1.40x faster**, or **28.5% less search time**.

### Why?

The query norm was repeated work. Moving it outside the loop reduces the amount of computation performed for every stored vector.

---

# Approach 5 — IVF with K-Means Clustering

### Idea

Exact search compares a query with every stored vector. IVF first groups vectors into clusters, then searches only the vectors in the cluster closest to the query.

### Index Build

1. Run K-means for 10 iterations.
2. Store one centroid for every cluster.
3. Assign every vector to its closest centroid.
4. Build an inverted list containing the vector indexes for each cluster.

```text
centroid 0 -> [vector indexes assigned to cluster 0]
centroid 1 -> [vector indexes assigned to cluster 1]
centroid 2 -> [vector indexes assigned to cluster 2]
```

### Query Path

```text
Query
  -> compare with all centroids
  -> choose the closest centroid
  -> read that centroid's inverted list
  -> search only vectors in that list
  -> return top K results
```

The current version searches one closest cluster. This is the first IVF version and is approximate: it can miss a true nearest neighbor that belongs to a nearby cluster.

### Query Benchmark — Dimension = 3, K = 1

|    Vectors | Clusters | Search Time |
| ---------: | -------: | ----------: |
|        100 |        1 |    0.016 ms |
|      1,000 |        1 |    0.091 ms |
|     10,000 |       10 |    0.039 ms |
|    100,000 |      100 |    0.025 ms |
|  1,000,000 |       20 | **1.184 ms** |

### Query-Speed Comparison at 1M Vectors

```text
Exact search with query norm once -> 24.643 ms
IVF with 20 clusters             ->  1.184 ms
```

The IVF query is approximately **20.8x faster**, or **95.2% less query time**.

### Current Limits

- K-means index building is single-threaded and expensive at large data sizes.
- Index-build time is excluded from the query benchmark above and has not yet been measured separately.
- Recall has not yet been compared with exact search.
- A 10M-vector, 20-cluster build was interrupted before completion.

---

# Current Benchmark Summary

```text
Approach 1: Hash-map storage       -> 2072.76 ms
Approach 2: Separate storage       ->  550.84 ms
Approach 3: Contiguous storage     ->  356.03 ms
Approach 4: Query norm once        ->  254.59 ms
Approach 5: IVF, 1M, 20 clusters   ->    1.18 ms

Approaches 1-4 are exact brute-force search.
Approach 5 is approximate IVF search.
```

We have improved the exact baseline and built the first approximate index.

---

# Current Limitation

IVF makes query search much faster by avoiding a full database scan, but the current index is not ready to evaluate at the 1B-vector goal:

```text
K-means build time is high at large N.
Recall has not been measured yet.
Only one cluster is searched per query.
```

# Next Stage

The next stage is measuring and improving the IVF index build.

The same process continues:

```text
Measure K-means build time separately from query time
    ↓
Learn thread partitioning, race conditions, and reduction
    ↓
Parallelize K-means training
    ↓
Measure IVF recall against exact search
```
