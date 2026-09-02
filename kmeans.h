#pragma once

#include <bits/stdc++.h>
using namespace std;

void kmeans(
    const vector<float>&embeddings,
    int num_vectors,
    int dimension,
    int num_clusters,
    vector<float>&centroids,
    vector<vector<int>>&inverted_list
);
