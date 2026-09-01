#include "vector_db.h"

unordered_map<string, int> id_to_index;
vector<string> ids;
vector<vector<float>> embeddings;

void insert(string str, vector<float> v) {

    if (id_to_index.find(str) == id_to_index.end()) {

        int index = embeddings.size();

        ids.push_back(str);
        embeddings.push_back(v);
        id_to_index[str] = index;
    }
}

float find_cosine_similarity(
    const vector<float>& a,
    const vector<float>& b
) {
    float numerator = 0.0f;
    float num1 = 0.0f;
    float num2 = 0.0f;

    int n = a.size();

    for (int i = 0; i < n; i++) {
        numerator += a[i] * b[i];
        num1 += a[i] * a[i];
        num2 += b[i] * b[i];
    }

    num1 = sqrt(num1);
    num2 = sqrt(num2);

    float deno = num1 * num2;

    return numerator / deno;
}

vector<pair<string, float>> search(vector<float> v, int k) {

    vector<pair<string, float>> ans;

    // Min heap:
    priority_queue<
        pair<float, string>,
        vector<pair<float, string>>,
        greater<pair<float, string>>
    > pq;

    // We do NOT iterate through the unordered_map.
    for (int i = 0; i < embeddings.size(); i++) {

        // Reference instead of copying the vector
        const vector<float>& temp = embeddings[i];

        float val = find_cosine_similarity(v, temp);

        if (pq.size() < k) {
            pq.push({val, ids[i]});
        }

        else if (pq.top().first < val) {
            pq.pop();
            pq.push({val, ids[i]});
        }
    }

    while (!pq.empty()) {
        ans.push_back({
            pq.top().second,
            pq.top().first
        });

        pq.pop();
    }

    reverse(ans.begin(), ans.end());

    return ans;
}

int size() {
    return embeddings.size();
}