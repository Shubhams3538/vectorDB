#include "vector_db.h"

unordered_map<string, int> id_to_index;
vector<string> ids;
vector<float> embeddings;

int dimension = 0;


// function to generate vector of some dimension
vector<float> generateVector(int dimension) {
    static random_device rd;
    static mt19937 gen(rd());
    static uniform_real_distribution<float> dis(0.0f, 1.0f);

    vector<float> v(dimension);

    for (int i = 0; i < dimension; i++) {
        v[i] = dis(gen);
    }

    return v;
}


void insert(const string& id, const vector<float>& v) {

    if (id_to_index.find(id) != id_to_index.end()) {
        return;
    }

    if (dimension == 0) {
        dimension = v.size();
    }

    if (v.size() != dimension) {
        return;
    }

    int index = ids.size();

    id_to_index[id] = index;
    ids.push_back(id);

    for (float x : v) {
        embeddings.push_back(x);
    }
}


void generate_random_database(int n, int dim) {

    dimension = dim;

    id_to_index.reserve(n);
    ids.reserve(n);
    embeddings.reserve(1LL * n * dim);

    for (int i = 0; i < n; i++) {
        string id = to_string(i);
        vector<float> v = generateVector(dim);

        insert(id, v);
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

    if (deno == 0.0f) {
        return 0.0f;
    }

    return numerator / deno;
}


vector<pair<string, float>> search(
    const vector<float>& v,
    int k
) {
    vector<pair<string, float>> ans;

    // min heap to find top k closest vectors
    priority_queue<
        pair<float, string>,
        vector<pair<float, string>>,
        greater<pair<float, string>>
    > pq;

    float query_norm = 0.0f;

    // calculating det(query vector) just once to save number of computation
    for (int i = 0; i < dimension; i++) {
        query_norm += v[i] * v[i];
    }

    query_norm = sqrt(query_norm);

    for (int i = 0; i < ids.size(); i++) {

        int start = i * dimension;

        float numerator = 0.0f;
        float vector_norm = 0.0f;

        for (int j = 0; j < dimension; j++) {

            float current = embeddings[start + j];

            numerator += v[j] * current;
            vector_norm += current * current;
        }

        vector_norm = sqrt(vector_norm);

        float deno = query_norm * vector_norm;

        float val = 0.0f;

        if (deno != 0.0f) {
            val = numerator / deno;
        }

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
    return ids.size();
}