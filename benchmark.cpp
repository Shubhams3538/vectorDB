#include "vector_db.h"
#include <chrono>
#include <iostream>
#include <random>

using namespace std;

int main() {

    cout << "Enter how many random vectors you want : " << endl;

    int n;
    cin >> n;

    int dim;

    cout << "Enter dimensions of each vector" << endl;
    cin >> dim;

    int k;

    cout << "Enter how many closest vectors you want : " << endl;
    cin >> k;

    generate_random_database(n, dim);

    vector<float> query = generateVector(dim);

    search(query, k);

    auto start = chrono::high_resolution_clock::now();

    auto result = search(query, k);

    auto end = chrono::high_resolution_clock::now();

    double time =
        chrono::duration<double, milli>(end - start).count();

    cout << "Vectors: " << n << endl;
    cout << "Dimension: " << dim << endl;
    cout << "K: " << k << endl;
    cout << "Search time: " << time << " ms\n";

    for (auto& x : result) {
        cout << x.first << " " << x.second << "\n";
    }

    return 0;
}