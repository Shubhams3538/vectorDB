#include "kmeans.h"


void kmeans(const vector<float>&embeddings,
int num_vectors,int dimensions , int num_clusters, vector<float>&centroids,  vector<vector<int>>&inverted_list){
    centroids.assign(num_clusters * dimensions, 0.0f);
    inverted_list.assign(num_clusters, {});

    for(int cluster = 0;cluster<num_clusters;cluster++){
        for(int dimension = 0;dimension<dimensions;dimension++){
            centroids[cluster * dimensions + dimension] =
                embeddings[cluster * dimensions + dimension];
        }
    }

    // to which cluster has been assigned to each vector
    vector<int>assignments(num_vectors);

    for(int iteration = 0; iteration < 10 ; iteration++){
        for(int vector_ind = 0;vector_ind < num_vectors;vector_ind++){
            // to figure out the closest centroid
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
        vector<float> sums(num_clusters * dimensions , 0.0f);
        vector<int> noofvectorsincluster(num_clusters , 0);
        for(int vector_ind = 0; vector_ind < num_vectors;vector_ind++){
            int clusterno = assignments[vector_ind];
            noofvectorsincluster[clusterno]++;

            // Add this vector to the sum for its assigned cluster.
            for(int d = 0;d<dimensions;d++){
                sums[clusterno * dimensions + d] += embeddings[vector_ind * dimensions + d];
            }
        }

        for(int cluster = 0;cluster < num_clusters;cluster++){
            if(noofvectorsincluster[cluster] == 0) continue;

            for(int d = 0;d<dimensions;d++){
                centroids[cluster * dimensions + d] = sums[cluster * dimensions + d] / noofvectorsincluster[cluster];
            }
        }
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
}
