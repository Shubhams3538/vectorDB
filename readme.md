# VectorDB

A vector database built from scratch in C++ with the long-term goal of efficiently searching **1 billion vectors in under 100 ms**.

### Approach

We are building it incrementally, starting with brute-force vector search and improving the architecture only when the current implementation reaches its limits.
Each stage is benchmarked to identify bottlenecks and understand why the next optimization is necessary.

### Benchmark — Stage 1

**Configuration**

- Vector dimension: `3`
- Similarity: `Cosine Similarity`
- Search: `Brute Force`
- Top-K: `2`

|    Vectors | Search Time |
| ---------: | ----------: |
|      1,000 |    0.806 ms |
|     10,000 |    5.625 ms |
|    100,000 |   18.919 ms |
| 10,000,000 | 2,072.76 ms |

### Observation

At **10 million vectors**, brute-force search already takes approximately **2.07 seconds**, showing that the current approach does not scale to billion-scale vector search.

A **100 million vector** test was also attempted, but the current `unordered_map<string, vector<float>>` representation became extremely memory-heavy and the test was stopped after several minutes.
