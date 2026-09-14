#include "vector_db.h"
#include <chrono>
#include <iostream>
#include <random>

using namespace std;

int main() {

    cout << "Enter how many random vectors you want : " << endl;

    int n;
    if (!(cin >> n) || n <= 0) {
        cerr << "Number of vectors must be a positive integer.\n";
        return 1;
    }

    int dim;

    cout << "Enter dimensions of each vector" << endl;
    if (!(cin >> dim) || dim <= 0) {
        cerr << "Vector dimension must be a positive integer.\n";
        return 1;
    }

    int k;

    cout << "Enter how many closest vectors you want : " << endl;
    if (!(cin >> k) || k <= 0) {
        cerr << "K must be a positive integer.\n";
        return 1;
    }

    int num_clusters;

    cout << "Enter how many clusters you want : " << endl;
    if (!(cin >> num_clusters) || num_clusters <= 0 || num_clusters > n) {
        cerr << "Number of clusters must be between 1 and " << n << ".\n";
        return 1;
    }

    auto start = chrono::high_resolution_clock::now();
    generate_random_database(n, dim);
    build_ivf(num_clusters);

    auto end = chrono::high_resolution_clock::now();
    double time = chrono::duration<double, milli>(end - start).count();

    cout << "Time taken to create Inverted File System : " << time << " ms\n";

    cout<<"1. Choose custom vector"<<endl;
    cout<<"2. Generate random vector"<<endl;
    int opt;
    if (!(cin >> opt) || (opt != 1 && opt != 2)) {
        cerr << "Please choose either 1 or 2.\n";
        return 1;
    }
    vector<float> query;

    if(opt == 1){
        cout << "Input " << dim
             << " vector values, separated by spaces or newlines: " << endl;
        query.resize(dim);

        for (int i = 0; i < dim; i++) {
            if (!(cin >> query[i])) {
                cerr << "Expected " << dim << " numeric vector values.\n";
                return 1;
            }
        }
    }
    else{
       query = generateVector(dim);
    }

    // Warm up the search path before measuring steady-state latency.
    search(query, k);

    start = chrono::high_resolution_clock::now();

    auto result = search(query, k);

    end = chrono::high_resolution_clock::now();

    time = chrono::duration<double, milli>(end - start).count();

    cout << "Vectors: " << n << endl;
    cout << "Dimension: " << dim << endl;
    cout << "K: " << k << endl;
    cout << "Search time: " << time << " ms\n";

    for (auto& x : result) {
        cout << x.first << " " << x.second << "\n";
    }

    return 0;
}
