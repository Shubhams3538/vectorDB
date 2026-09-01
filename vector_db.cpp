#include "vector_db.h"

unordered_map<string, vector<float>> db;

void insert(string str, vector<float> v) {
    if (db.find(str) == db.end()) {
        db[str] = v;
    }
}

float find_cosine_similarity(vector<float>& a, vector<float>& b) {
    float numerator = a[0] * b[0];
    float num1 = pow(a[0], 2);
    float num2 = pow(b[0], 2);

    int n = a.size();

    for (int i = 1; i < n; i++) {
        numerator += a[i] * b[i];
        num1 += pow(a[i], 2);
        num2 += pow(b[i], 2);
    }

    num1 = sqrt(num1);
    num2 = sqrt(num2);

    float deno = num1 * num2;

    return numerator / deno;
}

vector<pair<string, float>> search(vector<float> v, int k) {
    vector<pair<string, float>> ans;

    priority_queue<
        pair<float, string>,
        vector<pair<float, string>>,
        greater<pair<float, string>>
    > pq;

    for (auto it : db) {
        vector<float> temp = it.second;

        float val = find_cosine_similarity(v, temp);

        if (pq.size() < k) {
            pq.push({val, it.first});
        }
        else if (pq.top().first < val) {
            pq.pop();
            pq.push({val, it.first});
        }
    }

    while (!pq.empty()) {
        ans.push_back({pq.top().second, pq.top().first});
        pq.pop();
    }

    reverse(ans.begin(), ans.end());

    return ans;
}

int size() {
    return db.size();
}