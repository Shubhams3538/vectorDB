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

- Vector-to-centroid assignment, centroid accumulation, and the final assignment pass are now parallelized; centroid division and inverted-list construction remain serial.
- Database generation and IVF index-build time are now measured separately.
- Recall has not yet been compared with exact search.

---

# Approach 6 — Parallel Vector-to-Centroid Assignment

The most expensive loop in each K-means iteration assigns every vector to its closest centroid. This loop is now divided into non-overlapping vector ranges and executed by up to 10 worker threads.

```text
embeddings and centroids -> shared read-only
assignments              -> shared, disjoint index writes
each worker              -> one non-overlapping vector range
join all workers         -> continue to centroid recomputation
```

No mutex is required for `assignments` because each vector index is owned by exactly one worker. All workers are joined before centroids are recomputed.

### Correctness Validation

The serial and parallel implementations were run against the same embeddings and centroids during all 10 K-means iterations. Their complete assignment arrays matched exactly for:

- 7 vectors, covering fewer vectors than workers.
- 10,003 vectors, covering an uneven partition.
- 100,000 vectors, dimension 3, and 10 clusters.

### Preliminary Assignment Timing

For 100,000 vectors, dimension 3, 10 clusters, and 10 K-means iterations:

```text
Serial assignment total   -> 163.784 ms
Parallel assignment total ->  35.778 ms
Observed speedup           ->   4.58x
```

This is a preliminary, non-Release measurement of the assignment phase only. The validation path at the time deliberately executed both implementations, so its total IVF-build time was not a production performance result. Comparison instrumentation has since been removed as documented below.

---

# Approach 7 — Parallel Centroid Accumulation

Completed and checked on October 8, 2026. Each worker processes a disjoint range of vectors and updates its own centroid sums and cluster counts. Embeddings and assignments are shared read-only. After all workers join, their buffers are merged serially in fixed worker order; centroid division remains serial.

During validation, the serial helper and temporary execution timers were retained for comparison. Each K-means iteration ran both accumulation helpers on identical embeddings and assignments with separate zero-initialized output buffers. The parallel outputs were used to update centroids. The cleanup below removes this duplicate execution.

### Validation

- Both `app` and `benchmark` built successfully with `cmake --build build -j 4`.
- `git diff --check` passed.
- For 100,000 vectors, dimension 3, and 10 clusters, counts matched exactly and sums matched within tolerance across all 10 iterations.
- The same checks passed for 7 vectors, dimension 3, and 2 clusters, exercising a workload smaller than the 10-worker cap.
- Assignment arrays also matched exactly across all 10 iterations in both runs.

Floating-point addition order changes with threading. The original float-buffer comparison used absolute difference <= `1e-4 + 1e-4 * max(abs(serial_sum), abs(parallel_sum))`; counts require exact equality. The precision fix below now uses double buffers and a tighter comparison. These checks cover the tested inputs, not every possible input.

### Preliminary Timing — October 8, 2026

Measured on the local development machine using `/opt/homebrew/bin/g++-15` and the existing CMake build with an unset `CMAKE_BUILD_TYPE` (not a controlled Release benchmark). These are single-run totals across 10 K-means iterations using randomly generated vectors.

```text
100,000 vectors, dimension 3, 10 clusters, up to 10 workers:
Serial accumulation total     -> 14.667 ms
Parallel accumulation total   ->  6.706 ms
Time saved                    ->  7.961 ms
Observed accumulation speedup ->  2.19x
Serial assignment total       -> 156.571 ms
Parallel assignment total     ->  34.675 ms
Observed assignment speedup   ->  4.52x
Instrumented IVF build total  -> 233.563 ms

7 vectors, dimension 3, 2 clusters, 7 workers:
Serial accumulation total     -> 0.003 ms
Parallel accumulation total   -> 0.570 ms
Time saved                    -> -0.567 ms
Serial assignment total       -> 0.002 ms
Parallel assignment total     -> 0.564 ms
Instrumented IVF build total  -> 1.162 ms
```

The accumulation timer includes the threaded helper's private-buffer allocation, thread creation, joins, and merge. Output-buffer initialization and correctness comparisons are outside the accumulation timers. Thread overhead dominates the tiny workload. Total IVF build time includes running both serial and parallel implementations for validation, so it is not a production build-time measurement. These runs do not measure recall.

Reproduce the two input configurations:

```sh
printf '100000\n3\n10\n10\n2\n' | ./build/benchmark
printf '7\n3\n3\n2\n2\n' | ./build/benchmark
```

The input order is vector count, dimensions, search K, cluster count, and query option (`2` generates a random query). Random data and timings will vary between runs.

### Larger Run — Validation Failure (October 8, 2026)

User-reported run with 10,000,000 vectors, dimension 3, 5 clusters, and search K=5:

```text
Database generation            -> 4490.47 ms
Iteration 0, sum coordinate 1  -> serial=2.17392e+06, parallel=2.17232e+06
Reported IVF build duration    -> 1215.03 ms (aborted build)
Reported search duration       -> 0.001 ms (empty index; invalid search benchmark)
```

The displayed sum difference is approximately 1,600 (0.0736%), exceeding the comparison's approximately 217.4 tolerance. Long `float` accumulation chains and different serial/parallel addition orders are the likely cause; no double-precision reference comparison has been run for this input. Casting completed sums to `double` for comparison cannot recover precision lost during accumulation.

The mismatch returns early from `kmeans()` before inverted lists are populated. Because `kmeans()` and `build_ivf()` return `void`, the benchmark continues and prints build/search timings despite the incomplete index. This run supplies no valid completed-build speedup or search result.

This failure was resolved by the precision and failure-reporting changes below. The original random dataset was not saved, so the repeat uses the same configuration with newly generated data.

### Precision Fix and Successful Repeat — October 8, 2026

Both accumulation helpers now use `double` sums, including the worker-private buffers. Embeddings and final centroids remain `float`; division happens in double precision before converting the centroid back to float. The comparison now requires absolute difference <= `1e-9 + 1e-10 * max(abs(serial_sum), abs(parallel_sum))`, with exact counts and finite sums.

`kmeans()` and `build_ivf()` now return a boolean success result. Invalid K-means input or failed comparisons return false. The benchmark checks that result and exits with status 1 before reporting a completed build or attempting search.

Validation:

- Both build targets and `git diff --check` passed.
- A temporary standalone check compared both helpers against an independent long-double accumulation for 0, 1, 7, and 10,003 vectors, using signed, exactly representable fractional values and an empty cluster. Sums and counts matched the reference exactly.
- Invalid input returned false. Injecting a NaN into a one-vector database made `build_ivf()` return false and left the inverted list empty.
- The full 10,000,000-vector benchmark completed all 10 iterations: assignments and counts matched exactly, and double sums passed the tighter tolerance.
- The 7-vector benchmark also completed and passed all comparisons.

Same local compiler and non-Release build conditions as above; single runs, with timings totaling 10 iterations:

```text
10,000,000 vectors, dimension 3, 5 clusters, K=5, up to 10 workers:
Database generation           -> 4389.9 ms
Serial assignment total       -> 8430.66 ms
Parallel assignment total     -> 1666.86 ms
Observed assignment speedup   -> 5.06x
Serial accumulation total     -> 1538.25 ms
Parallel accumulation total   -> 628.251 ms
Time saved                    -> 909.999 ms
Observed accumulation speedup -> 2.45x
Instrumented IVF build total  -> 13587 ms
Search duration               -> 45.679 ms (one random query; recall unmeasured)

7 vectors, dimension 3, 2 clusters, K=3, 7 workers:
Serial assignment total       -> 0.003 ms
Parallel assignment total     -> 0.545 ms
Serial accumulation total     -> 0.003 ms
Parallel accumulation total   -> 0.544 ms
Time saved                    -> -0.541 ms
Instrumented IVF build total  -> 1.119 ms
Search duration               -> 0.003 ms
```

Large-run input:

```sh
printf '10000000\n3\n5\n5\n2\n' | ./build/benchmark
```

The large successful repeat supports the float-accumulation diagnosis, but it does not reproduce the original random dataset. That run executed both implementations; its total build time includes comparison instrumentation. Tiny-run threading overhead was higher than serial work.

### User Benchmark — 10 Million Vectors, 10 Clusters (October 8, 2026)

User-reported output from `./build/benchmark`, dimension 3, K=5, 10 clusters, random query. This run was not independently repeated by the agent; build mode is assumed to be the existing non-Release build.

```text
Database generation           -> 4698.68 ms
Serial assignment total       -> 15613.3 ms
Parallel assignment total     -> 2935.37 ms
Assignment speedup            -> 5.31902x
Serial accumulation total     -> 1554.53 ms
Parallel accumulation total   -> 561.137 ms
Accumulation time saved        -> 993.393 ms
Accumulation speedup          -> 2.77032x
Instrumented IVF build total  -> 22701.3 ms
Search duration               -> 34.392 ms
```

Assignments and counts matched exactly, and sums passed the tolerance across all 10 iterations. This is one query after the benchmark's warm-up, not a latency distribution or a recall measurement.

The goal remains 1 billion vectors in under 100 ms. A simple linear projection at the same 10 clusters, dimension, and hardware gives `34.392 ms * 100 = 3439.2 ms` (about 3.44 seconds), roughly 34.4 times the target. This is an estimate, not a measured billion-vector result: memory capacity, cluster imbalance, cache behavior, and the selected query can change scaling.

Search scans only the closest cluster. Balanced clusters would contain about 1 million vectors each at 10 million vectors / 10 clusters, versus 100 million each at 1 billion / 10 clusters; actual cluster populations were not logged. More clusters could reduce candidates, but add centroid-selection and K-means build work, and single-cluster recall must be measured before treating speed as success. With approximately 1,000 balanced clusters at 1 billion vectors, average cluster population would remain around 1 million; this does not guarantee the measured 34 ms latency.

Raw 3D float embeddings alone need 12 GB at 1 billion vectors, before IDs, the ID map, inverted lists, assignments, and other buffers. Existing `int` flattened offsets such as `int start = i * dimension` in search also overflow for sufficiently large indexes at dimension 3; address index widths before scaling to that size. Higher-dimensional vector workloads require separate measurements.

### Parallel-Path Cleanup — October 8, 2026

Removed both single-threaded comparison helpers, duplicate assignment/sum/count buffers, serial-versus-parallel validation loops, all phase-comparison timers, and their speedup output. Each iteration now runs only parallel assignment and parallel accumulation. Double sums, non-finite-sum rejection, and boolean build-failure reporting remain. The necessary serial buffer merge, centroid division, final assignment, and inverted-list construction are unchanged.

Validation:

- Both targets built and `git diff --check` passed.
- A temporary before/after check produced identical final centroids and inverted lists for deterministic datasets of 1, 7, and 10,003 vectors. Every vector appeared exactly once in the lists.
- Invalid input and a NaN sum still returned failure.
- An end-to-end run completed with 100,000 random vectors, dimension 3, 10 clusters, and K=5 using the existing non-Release build.

```text
Database generation -> 62.1 ms
IVF build           -> 60.262 ms
Search              -> 0.301 ms (one random query after warm-up)
```

Reproduce the configuration with `printf '100000\n3\n5\n10\n2\n' | ./build/benchmark`. This is a single preliminary run; no 10-million-vector benchmark was repeated for this cleanup. The benchmark retains its existing database-generation, complete IVF-build, and search timers. Historical serial/parallel comparison results above remain as evidence, but no comparison code runs in the current implementation.

### Parallel Final Assignment — October 8, 2026

The final reassignment after the tenth centroid update now calls the existing `multi_threaded_cluster_assignment()` helper. Its workers write disjoint assignment ranges and join before the unchanged serial inverted-list fill. The old final assignment loop is commented out and does not execute; it can be deleted during cleanup.

Validation: both targets built, whitespace checks passed, and a temporary deterministic before/after check produced identical centroids and inverted lists for 1, 7, and 10,003 vectors. Every vector appeared exactly once. Invalid input and non-finite sums still reported failure.

An end-to-end benchmark with 100,000 random vectors, dimension 3, 10 clusters, K=5, and a random query completed in the existing non-Release build:

```text
Database generation -> 34.817 ms
IVF build           -> 48.714 ms
Search              -> 0.380 ms
```

This single run uses newly generated random data; it is not a controlled speedup comparison with the previous cleanup run. Recall remains unmeasured.

### Documentation Workflow

After each completed implementation part and benchmark run, update this README with the behavior, validation, input configuration, build mode, measured subsystem, timings, limitations, and exact next step. Keep earlier measurements labeled with their original conditions.

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

We have improved the exact baseline, built the first approximate index, and parallelized its vector-assignment and centroid-accumulation phases.

---

# Current Limitation

IVF makes query search much faster by avoiding a full database scan, but the current index is not ready to evaluate at the 1B-vector goal:

```text
K-means build time is high at large N.
Buffer merging, centroid division, and inverted-list construction are still serial.
Controlled Release benchmarks and query recall evaluation remain pending.
Recall has not been measured yet.
Only one cluster is searched per query.
```

# Next Stage

The final assignment pass is parallelized and validated. The next step is a controlled Release benchmark of the current implementation, followed by recall evaluation against exact search.

The same process continues:

```text
Parallel vector assignment with disjoint output ranges [done]
    ↓
Give each worker private centroid sums and counts [done]
    ↓
Reduce worker-local results in a fixed order [done]
    ↓
Parallelize the final assignment pass [done]
    ↓
Run controlled Release benchmarks
    ↓
Measure IVF recall against exact search
```
