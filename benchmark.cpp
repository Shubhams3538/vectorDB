#include "vector_db.h"
#include <chrono>
#include <iostream>
#include <random>
#include <string>

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
    // calculate the time to generate random db
    auto start = chrono::high_resolution_clock::now();
    generate_random_database(n, dim);
    auto end = chrono::high_resolution_clock::now();
    double time = chrono::duration<double, milli>(end - start).count();

    cout << "Time taken to generate random DB: " << time << " ms\n";

    start = chrono::high_resolution_clock::now();
    if (!build_ivf(num_clusters)) {
        cerr << "IVF build failed; stopping benchmark before search.\n";
        return 1;
    }

    end = chrono::high_resolution_clock::now();
    time = chrono::duration<double, milli>(end - start).count();
    cout << "Time taken to create Inverted File System : " << time << " ms\n";

    while (true) {
        cout << "\n1. Choose custom vector\n";
        cout << "2. Generate random vector\n";
        cout << "Type exit or quit to stop.\n";
        string opt;
        if (!(cin >> opt) || opt == "exit" || opt == "quit") {
            break;
        }
        if (opt != "1" && opt != "2") {
            cerr << "Please choose 1 or 2, or type exit or quit.\n";
            continue;
        }
        vector<float> query;

        if(opt == "1"){
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

        // Warm up both paths before timing the same query.
        search(query, k);
        exact_search(query, k);

        start = chrono::high_resolution_clock::now();

        auto result = search(query, k);

        end = chrono::high_resolution_clock::now();

        time = chrono::duration<double, milli>(end - start).count();


        start = chrono::high_resolution_clock::now();

        auto result2 = exact_search(query, k);

        end = chrono::high_resolution_clock::now();

        double time2 = chrono::duration<double, milli>(end - start).count();

        cout << "Vectors: " << n << endl;
        cout << "Dimension: " << dim << endl;
        cout << "K: " << k << endl;
        cout << "Search time using IVF & Clustering: " << time << " ms\n";
        cout << "Exact search time: " << time2 << " ms\n";
        if (time > 0.0) {
            cout << "Search speedup (exact / IVF): " << time2 / time << "x\n";
        }

        unordered_set<string> exact_ids;
        for (const auto& x : result2) {
            exact_ids.insert(x.first);
        }
        int shared_ids = 0;
        for (const auto& x : result) {
            if (exact_ids.count(x.first)) {
                shared_ids++;
            }
        }

        // When K exceeds the database size, use the number of available neighbors.
        if (!result2.empty()) {
            cout << "Recall@" << k << " (ID overlap): "
                 << 100.0 * shared_ids / result2.size() << "% ("
                 << shared_ids << '/' << result2.size() << ")\n";

            // Equal scores at the exact top-K boundary can select different valid IDs.
            float cutoff = result2.back().second;
            int required_above_cutoff = 0;
            int found_above_cutoff = 0;
            int found_at_cutoff = 0;
            for (const auto& x : result2) {
                if (x.second > cutoff) {
                    required_above_cutoff++;
                }
            }
            for (const auto& x : result) {
                if (x.second > cutoff && exact_ids.count(x.first)) {
                    found_above_cutoff++;
                }
                else if (x.second == cutoff) {
                    found_at_cutoff++;
                }
            }
            int boundary_slots = int(result2.size()) - required_above_cutoff;
            int credited = found_above_cutoff + min(found_at_cutoff, boundary_slots);
            cout << "Recall@" << k << " (allowing equal-score boundary ties): "
                 << 100.0 * credited / result2.size() << "% ("
                 << credited << '/' << result2.size() << ")\n";
        }

        cout << setprecision(9);
        cout << "Vectors given by IVF: " << endl;
        for (auto& x : result) {
            cout << x.first << " " << x.second << "\n";
        }
        // printing the result by exact search
        cout << "\nVectors given by exact search: " << endl;

        for (auto& x : result2) {
            cout << x.first << " " << x.second << "\n";
        }
    }

    return 0;
}
