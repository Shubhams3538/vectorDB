#pragma once

#include <bits/stdc++.h>
using namespace std;

// Maps string ID -> index in ids/embeddings
extern unordered_map<string, int> id_to_index;

// Stores IDs separately
extern vector<string> ids;

// Stores all vectors
extern vector<vector<float>> embeddings;

void insert(string str, vector<float> v);

float find_cosine_similarity(
    const vector<float>& a,
    const vector<float>& b
);

vector<pair<string, float>> search(vector<float> v, int k);

int size();