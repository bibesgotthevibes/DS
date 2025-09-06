#include <iostream>
#include <memory>
#include <string>
#include <chrono>
#include <thread>
#include <random>
#include <grpcpp/grpcpp.h>
#include "graph.grpc.pb.h"

using namespace std;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using graph::GraphProcessor;
using graph::Graph;
using graph::SubmissionResponse;
using graph::IndependentSetQuery;
using graph::MatchingQuery;
using graph::QueryResponse;

class GraphClient {
public:
    GraphClient(shared_ptr<Channel> channel, int client_id)
        : stub_(GraphProcessor::NewStub(channel)), client_id_(client_id) {}

    // Submit a graph to the server
    bool SubmitGraph(const unordered_map<int, vector<int>>& adj_lists) {
        auto start = chrono::high_resolution_clock::now();

        Graph request;
        request.set_client_id(client_id_);

        for (const auto& [vertex, neighbors] : adj_lists) {
            auto& adj_list = (*request.mutable_adjacency_lists())[vertex];
            for (int neighbor : neighbors) {
                adj_list.add_neighbors(neighbor);
            }
        }

        SubmissionResponse response;
        ClientContext context;

        Status status = stub_->SubmitGraph(&context, request, &response);

        auto end = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::microseconds>(end - start);

        if (status.ok()) {
            cout << "[Client " << client_id_ << "] Graph submitted in " << duration.count() << "μs: " << response.message() << endl;
            return response.success();
        } else {
            cout << "[Client " << client_id_ << "] Error submitting graph: " << status.error_message() << endl;
            return false;
        }
    }

    // Query if there exists an independent set of size k
    bool QueryIndependentSet(int k) {
        auto start = chrono::high_resolution_clock::now();

        IndependentSetQuery request;
        request.set_size_threshold(k);

        QueryResponse response;
        ClientContext context;

        Status status = stub_->HasIndependentSet(&context, request, &response);

        auto end = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::microseconds>(end - start);

        if (status.ok()) {
            cout << "[Client " << client_id_ << "] Independent set query (k=" << k << ") in " << duration.count() << "μs: " << response.message() << endl;
            return response.result();
        } else {
            cout << "[Client " << client_id_ << "] Error querying independent set: " << status.error_message() << endl;
            return false;
        }
    }

    // Query if there exists a matching of size k
    bool QueryMatching(int k) {
        auto start = chrono::high_resolution_clock::now();

        MatchingQuery request;
        request.set_size_threshold(k);

        QueryResponse response;
        ClientContext context;

        Status status = stub_->HasMatching(&context, request, &response);

        auto end = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::microseconds>(end - start);

        if (status.ok()) {
            cout << "[Client " << client_id_ << "] Matching query (k=" << k << ") in " << duration.count() << "μs: " << response.message() << endl;
            return response.result();
        } else {
            cout << "[Client " << client_id_ << "] Error querying matching: " << status.error_message() << endl;
            return false;
        }
    }

private:
    unique_ptr<GraphProcessor::Stub> stub_;
    int client_id_;
};

// Generate a random graph
unordered_map<int, vector<int>> generateRandomGraph(int num_vertices, int num_edges, int seed) {
    unordered_map<int, vector<int>> graph;
    random_device rd;
    mt19937 gen(rd() + seed);
    uniform_int_distribution<> vertex_dist(0, num_vertices - 1);

    // Initialize all vertices
    for (int i = 0; i < num_vertices; i++) {
        graph[i] = vector<int>();
    }

    // Add random edges
    int edges_added = 0;
    while (edges_added < num_edges) {
        int v1 = vertex_dist(gen);
        int v2 = vertex_dist(gen);

        if (v1 != v2) {
            // Check if edge already exists
            bool edge_exists = false;
            for (int neighbor : graph[v1]) {
                if (neighbor == v2) {
                    edge_exists = true;
                    break;
                }
            }

            if (!edge_exists) {
                graph[v1].push_back(v2);
                graph[v2].push_back(v1);
                edges_added++;
            }
        }
    }

    return graph;
}

int main(int argc, char** argv) {
    cout << "=== Performance Client 1 (Random Graphs) Starting ===" << endl;

    // Connect to server
    GraphClient client(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()), 1);

    // Wait for server to be ready
    this_thread::sleep_for(chrono::seconds(5));

    // Test with different graph sizes
    vector<pair<int, int>> test_cases = {
        {10, 15},   // Small graph
        {20, 30},   // Medium graph
        {30, 45},   // Large graph
        {40, 60},   // Very large graph
        {50, 75}    // Extra large graph
    };

    for (int i = 0; i < test_cases.size(); i++) {
        auto [vertices, edges] = test_cases[i];

        cout << "\n=== Test " << (i+1) << ": Random graph with " << vertices << " vertices and " << edges << " edges ===" << endl;

        // Generate and submit graph
        auto graph = generateRandomGraph(vertices, edges, i + 1);
        client.SubmitGraph(graph);
        this_thread::sleep_for(chrono::milliseconds(100));

        // Test independent set queries
        for (int k = 1; k <= min(5, vertices/2); k++) {
            client.QueryIndependentSet(k);
            this_thread::sleep_for(chrono::milliseconds(50));
        }

        // Test matching queries
        for (int k = 1; k <= min(5, vertices/2); k++) {
            client.QueryMatching(k);
            this_thread::sleep_for(chrono::milliseconds(50));
        }

        this_thread::sleep_for(chrono::milliseconds(200));
    }

    cout << "\n=== Performance Client 1 Completed ===" << endl;
    return 0;
}
