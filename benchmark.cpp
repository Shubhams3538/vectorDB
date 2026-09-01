#include "vector_db.h"
#include <chrono>
#include <iostream>
#include <random>

using namespace std;


// function to generate vector of some dimension
vector<float> generateVector(int dimension){
    static random_device rd;
    static mt19937 gen(rd());
    static uniform_real_distribution<float>dis(0.0f,1.0f);

    vector<float>v(dimension);

    for(int i = 0;i<dimension;i++){
        v[i] = dis(gen);
    }
    return v;
}



int main() {

    cout<<"Enter how many random vectors you want : "<<endl;
    int n;
    cin>>n;


    for(int i = 0;i<n;i++){
        vector<float> temp = generateVector(3);
        insert(to_string(i) , temp);
    }

    vector<float> query = {1, 2, 3};

    auto start = chrono::high_resolution_clock::now();

    auto result = search(query, 2);

    auto end = chrono::high_resolution_clock::now();

    double time = chrono::duration<double, milli>(end - start).count();

    cout << "Search time: " << time << " ms\n";

    for (auto &x : result) {
        cout << x.first << " " << x.second << "\n";
    }
}