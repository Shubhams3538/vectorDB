#pragma once

#include <bits/stdc++.h>
using namespace std;

// Maps string ID -> index in ids/embeddings
extern unordered_map<string, int> id_to_index;

// 
extern vector<string> ids;

// Stores all vector values in one contiguous array
extern vector<float> embeddings;

extern int dimension;

void insert(const string& id, const vector<float>& v);

// function to generate vector of some dimension
vector<float> generateVector(int dimension);

void generate_random_database(int n, int dim);

void build_ivf(int num_clusters);

float find_cosine_similarity(
    const vector<float>& a,
    const vector<float>& b
);

vector<pair<string, float>> search(
    const vector<float>& v,
    int k
);

int size();
