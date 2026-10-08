#include "kmeans.h"
#include <thread>

void single_threaded_cluster_assignment(int num_vectors, int dimensions, const vector<float>& embeddings, const vector<float>& centroids, vector<int>& assignments, int num_clusters){
    for(int vector_ind = 0;vector_ind < num_vectors;vector_ind++){
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

            // the next step is to parallelize the whole k means code we will start from here

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
};

void single_threaded_centroid_accumulation(
    int num_vectors,
    int dimensions,
    const vector<float>& embeddings,
    const vector<int>& assignments,
    vector<double>& sums,
    vector<int>& noofvectorsincluster
) {
    for (int vector_ind = 0; vector_ind < num_vectors; vector_ind++) {
        int clusterno = assignments[vector_ind];
        noofvectorsincluster[clusterno]++;
        for (int d = 0; d < dimensions; d++) {
            sums[clusterno * dimensions + d] +=
                embeddings[vector_ind * dimensions + d];
        }
    }
}

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

    // to which cluster has been assigned to each vector
    // different vectors one for single threaded and one for multi to check
    // performance gain
    vector<int>assignments(num_vectors);
    vector<int>parallel_assignments(num_vectors);
    double serial_assignment_time = 0.0;
    double parallel_assignment_time = 0.0;
    // Temporary serial/parallel accumulation comparison across all iterations.
    double serial_accumulation_time = 0.0;
    double parallel_accumulation_time = 0.0;

    for(int iteration = 0; iteration < 10 ; iteration++){

        auto assignment_start = chrono::steady_clock::now();
        single_threaded_cluster_assignment(num_vectors , dimensions , embeddings ,centroids,assignments,num_clusters);
        auto assignment_end = chrono::steady_clock::now();
        serial_assignment_time += chrono::duration<double, milli>(assignment_end - assignment_start).count();

        assignment_start = chrono::steady_clock::now();
        multi_threaded_cluster_assignment(num_vectors , dimensions , embeddings ,centroids,parallel_assignments,num_clusters);
        assignment_end = chrono::steady_clock::now();
        parallel_assignment_time += chrono::duration<double, milli>(assignment_end - assignment_start).count();

        for(int vector_ind = 0; vector_ind < num_vectors; vector_ind++){
            if(assignments[vector_ind] != parallel_assignments[vector_ind]){
                cerr << "Assignment mismatch in iteration " << iteration
                     << " at vector " << vector_ind
                     << ": serial=" << assignments[vector_ind]
                     << ", parallel=" << parallel_assignments[vector_ind]
                     << '\n';
                return false;
            }
        }



        vector<double> sums(num_clusters * dimensions, 0.0);
        vector<int> noofvectorsincluster(num_clusters , 0);

        vector<double> serial_sums(num_clusters * dimensions, 0.0);
        vector<int> serial_counts(num_clusters, 0);

        auto accumulation_start = chrono::steady_clock::now();
        single_threaded_centroid_accumulation(
            num_vectors, dimensions, embeddings, assignments, serial_sums, serial_counts);
        auto accumulation_end = chrono::steady_clock::now();
        serial_accumulation_time +=
            chrono::duration<double, milli>(accumulation_end - accumulation_start).count();

        accumulation_start = chrono::steady_clock::now();
        multi_threaded_centroid_accumulation(
            num_vectors, dimensions, embeddings, assignments, sums, noofvectorsincluster);
        accumulation_end = chrono::steady_clock::now();
        parallel_accumulation_time +=
            chrono::duration<double, milli>(accumulation_end - accumulation_start).count();

        if (serial_counts != noofvectorsincluster) {
            cerr << "Accumulation count mismatch in iteration " << iteration << '\n';
            return false;
        }
        // Threading changes floating-point addition order, so sums need a tolerance.
        for (size_t i = 0; i < sums.size(); i++) {
            double difference = abs(serial_sums[i] - sums[i]);
            double tolerance = 1e-9 + 1e-10 * max(abs(serial_sums[i]), abs(sums[i]));
            if (!isfinite(serial_sums[i]) || !isfinite(sums[i]) || difference > tolerance) {
                cerr << "Accumulation sum mismatch in iteration " << iteration
                     << " at coordinate " << i << ": serial=" << serial_sums[i]
                     << ", parallel=" << sums[i] << '\n';
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

    cout << "Serial and parallel assignments matched for all 10 iterations.\n";
    cout << "Total serial cluster-assignment time: "
         << serial_assignment_time << " ms\n";
    cout << "Total parallel cluster-assignment time: "
         << parallel_assignment_time << " ms\n";

    if(parallel_assignment_time > 0.0){
        cout << "Cluster-assignment speedup: "
             << serial_assignment_time / parallel_assignment_time
             << "x\n";
    }

    cout << "Serial and parallel accumulation counts matched exactly; sums matched "
         << "within tolerance for all 10 iterations.\n";
    cout << "Total serial centroid-accumulation time: "
         << serial_accumulation_time << " ms\n";
    cout << "Total parallel centroid-accumulation time (including buffers, threads, joins, merge): "
         << parallel_accumulation_time << " ms\n";
    cout << "Accumulation time saved (serial - parallel): "
         << serial_accumulation_time - parallel_accumulation_time << " ms\n";
    if (parallel_accumulation_time > 0.0) {
        cout << "Centroid-accumulation speedup: "
             << serial_accumulation_time / parallel_accumulation_time << "x\n";
    }

    // The final centroid update changes centroid positions, so assign every
    // vector once more before building the final inverted lists.
    for(int vector_ind = 0;vector_ind < num_vectors;vector_ind++){
        int closest_cluster = 0;
        float closest_dist = 0.0f;

        for(int d = 0;d < dimensions;d++){
            float diff = embeddings[vector_ind * dimensions + d] - centroids[d];
            closest_dist += diff * diff;
        }

        for(int cluster = 1; cluster < num_clusters;cluster++){
            float temp_dist = 0.0f;

            for(int d = 0;d<dimensions;d++){
                float diff =
                    embeddings[vector_ind * dimensions + d] -
                    centroids[cluster * dimensions + d];

                temp_dist += diff * diff;
            }

            if(temp_dist < closest_dist){
                closest_dist = temp_dist;
                closest_cluster = cluster;
            }
        }

        assignments[vector_ind] = closest_cluster;
    }

      // centeroid -> [indexes of each vector in the particular cluster]
      for(int i = 0;i<num_vectors;i++){
         inverted_list[assignments[i]].push_back(i);
      }
      return true;
}
