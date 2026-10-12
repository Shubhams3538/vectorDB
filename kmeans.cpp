#include "kmeans.h"
#include <thread>

void multi_threaded_centroid_accumulation(
    int num_vectors,
    int dimensions,
    const vector<float>& embeddings,
    const vector<int>& assignments,
    vector<double>& sums,
    vector<int>& noofvectorsincluster
) {
    if (num_vectors <= 0) return;
    const int noofthreads = min(10, num_vectors);
    vector<thread> threads;
    threads.reserve(noofthreads);
    int no_of_vectorsforeach_thread = (num_vectors + noofthreads - 1) / noofthreads;

    // to avoid race condition giving each thread it's own copy of sum and count
    vector<vector<double>> thread_sums(noofthreads, vector<double>(sums.size(), 0.0));
    vector<vector<int>> thread_count(noofthreads, vector<int>(noofvectorsincluster.size(), 0));

    for (int t = 0; t < noofthreads; t++) {
        int start = t * no_of_vectorsforeach_thread;
        int end = min(start + no_of_vectorsforeach_thread, num_vectors);
        if (start >= end) break;

        threads.emplace_back([t, start, end, &assignments, &dimensions, &embeddings,
                              &thread_sums, &thread_count]() {
            for (int vector_ind = start; vector_ind < end; vector_ind++) {
                int clusterno = assignments[vector_ind];
                thread_count[t][clusterno]++;

                // Add this vector to the sum for its assigned cluster.
                for (int d = 0; d < dimensions; d++) {
                    thread_sums[t][clusterno * dimensions + d] +=
                        embeddings[vector_ind * dimensions + d];
                }
            }
        });
    }

    for (auto &th : threads) {
        th.join();
    }

    // combining all the sums in single vector sums by accumulating everything done by each thread
    for (int i = 0; i < noofthreads; i++) {
        for (int j = 0; j < sums.size(); j++) {
            sums[j] += thread_sums[i][j];
        }
    }

    // similary counting how many many vectors are assigned to each cluster and combining the result done
    // by each thread into a single noofvectorsincluster array
    for (int i = 0; i < noofthreads; i++) {
        for (int j = 0; j < noofvectorsincluster.size(); j++) {
            noofvectorsincluster[j] += thread_count[i][j];
        }
    }
}

void multi_threaded_cluster_assignment(int num_vectors, int dimensions, const vector<float>& embeddings, const vector<float>& centroids, vector<int>& assignments, int num_clusters){
    if (num_vectors <= 0) {
        return;
    }

    const int noofthreads = min(10, num_vectors);
    vector<thread>threads;
    threads.reserve(noofthreads);

    // ceil of chunk size
    int chunksize = (num_vectors + noofthreads  - 1 ) / noofthreads;
    for(int t = 0; t < noofthreads;t++){
        int start = t * chunksize;
        int end = min(start + chunksize , num_vectors);
        if(start >= end) break;

        threads.emplace_back([start, end, dimensions, &embeddings, &centroids, num_clusters, &assignments](){

            for(int vector_ind = start;vector_ind < end;vector_ind++){
            int closest_cluster = 0;
            // findig minimum distance
            float closest_dist = 0.0f;
            // to figure out the eucladian distance between the 0th cluster and current vector
            for(int d = 0;d < dimensions;d++){
                float diff = embeddings[vector_ind * dimensions + d] - centroids[d];
                closest_dist += diff * diff;
            }

            // finding eucladian distance between the current vector and remaining clusters and picking the closest
            // cluster to this vector
            for(int cluster = 1; cluster < num_clusters;cluster++){
                float temp_dist = 0;
                for(int d = 0;d<dimensions;d++){
                    float diff = embeddings[vector_ind * dimensions + d] - centroids[cluster * dimensions + d];
                    temp_dist += diff * diff;
                }
                if(temp_dist < closest_dist){
                    closest_dist = temp_dist;
                    closest_cluster = cluster;
                }
            }
            assignments[vector_ind] = closest_cluster;
          }
        });
    }

    // join the threads
    for(auto &th:threads){
        th.join();
    }

};

bool kmeans(const vector<float>&embeddings,
int num_vectors,int dimensions , int num_clusters, vector<float>&centroids,  vector<vector<int>>&inverted_list){
    if (num_vectors <= 0 || dimensions <= 0 || num_clusters <= 0 ||
        num_clusters > num_vectors ||
        embeddings.size() != size_t(num_vectors) * size_t(dimensions)) {
        cerr << "Invalid K-means input.\n";
        return false;
    }
    centroids.assign(num_clusters * dimensions, 0.0f);
    inverted_list.assign(num_clusters, {});

    for(int cluster = 0;cluster<num_clusters;cluster++){
        for(int dimension = 0;dimension<dimensions;dimension++){
            centroids[cluster * dimensions + dimension] =
                embeddings[cluster * dimensions + dimension];
        }
    }

    // Cluster assigned to each vector by the parallel helper.
    vector<int> assignments(num_vectors);

    for (int iteration = 0; iteration < 10; iteration++) {
        multi_threaded_cluster_assignment(
            num_vectors, dimensions, embeddings, centroids, assignments, num_clusters);

        vector<double> sums(num_clusters * dimensions, 0.0);
        vector<int> noofvectorsincluster(num_clusters, 0);
        multi_threaded_centroid_accumulation(
            num_vectors, dimensions, embeddings, assignments, sums, noofvectorsincluster);

        // Keep failure reporting for invalid sums without comparison instrumentation.
        for (double sum : sums) {
            if (!isfinite(sum)) {
                cerr << "Non-finite centroid sum in iteration " << iteration << '\n';
                return false;
            }
        }

        for(int cluster = 0;cluster < num_clusters;cluster++){
            if(noofvectorsincluster[cluster] == 0) continue;

            for(int d = 0;d<dimensions;d++){
                centroids[cluster * dimensions + d] = static_cast<float>(
                    sums[cluster * dimensions + d] / noofvectorsincluster[cluster]);
            }
        }
    }

    // The final centroid update changes centroid positions, so assign every
    // vector once more before building the final inverted lists.
    auto final_assignment_start = chrono::steady_clock::now();
    multi_threaded_cluster_assignment(
        num_vectors, dimensions, embeddings, centroids, assignments, num_clusters);
    auto final_assignment_end = chrono::steady_clock::now();
    double final_assignment_time = chrono::duration<double, milli>(
        final_assignment_end - final_assignment_start).count();
    cout << "Parallel final cluster-assignment time: "
         << final_assignment_time << " ms\n";

      // centeroid -> [indexes of each vector in the particular cluster]
      for(int i = 0;i<num_vectors;i++){
         inverted_list[assignments[i]].push_back(i);
      }
      return true;
}
