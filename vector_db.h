#pragma once

#include <bits/stdc++.h>
using namespace std;

extern unordered_map<string, vector<float>> db;

void insert(string str, vector<float> v);

float find_cosine_similarity(vector<float>& a, vector<float>& b);

vector<pair<string, float>> search(vector<float> v, int k);

int size();