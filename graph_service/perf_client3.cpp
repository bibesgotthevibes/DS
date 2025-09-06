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

// Generate a bipartite graph
unordered_map<int, vector<int>> generateBipartiteGraph(int left_size, int right_size, int num_edges, int seed) {
    unordered_map<int, vector<int>> graph;
    random_device rd;
    mt19937 gen(rd() + seed);
    uniform_int_distribution<> left_dist(0, left_size - 1);
    uniform_int_distribution<> right_dist(left_size, left_size + right_size - 1);

    // Initialize all vertices
    for (int i = 0; i < left_size + right_size; i++) {
        graph[i] = vector<int>();
    }

    // Add random edges between left and right partitions
    int edges_added = 0;
    while (edges_added < num_edges) {
        int left_vertex = left_dist(gen);
        int right_vertex = right_dist(gen);

        // Check if edge already exists
        bool edge_exists = false;
        for (int neighbor : graph[left_vertex]) {
            if (neighbor == right_vertex) {
                edge_exists = true;
                break;
            }
        }

        if (!edge_exists) {
            graph[left_vertex].push_back(right_vertex);
            graph[right_vertex].push_back(left_vertex);
            edges_added++;
        }
    }

    return graph;
}

// Generate a cycle graph
unordered_map<int, vector<int>> generateCycleGraph(int num_vertices) {
    unordered_map<int, vector<int>> graph;

    // Initialize all vertices
    for (int i = 0; i < num_vertices; i++) {
        graph[i] = vector<int>();
    }

    // Connect vertices in a cycle
    for (int i = 0; i < num_vertices; i++) {
        int next = (i + 1) % num_vertices;
        graph[i].push_back(next);
        graph[next].push_back(i);
    }

    return graph;
}

// Generate a path graph
unordered_map<int, vector<int>> generatePathGraph(int num_vertices) {
    unordered_map<int, vector<int>> graph;

    // Initialize all vertices
    for (int i = 0; i < num_vertices; i++) {
        graph[i] = vector<int>();
    }

    // Connect vertices in a path
    for (int i = 0; i < num_vertices - 1; i++) {
        graph[i].push_back(i + 1);
        graph[i + 1].push_back(i);
    }

    return graph;
}

int main(int argc, char** argv) {
    cout << "=== Performance Client 3 (Mixed Graphs) Starting ===" << endl;

    // Connect to server
    GraphClient client(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()), 3);

    // Wait for server to be ready
    this_thread::sleep_for(chrono::seconds(9));

    // Test with different graph types
    vector<pair<string, unordered_map<int, vector<int>>>> test_graphs = {
        {"Bipartite (10,10,20)", generateBipartiteGraph(10, 10, 20, 1)},
        {"Cycle (15)", generateCycleGraph(15)},
        {"Path (20)", generatePathGraph(20)},
        {"Bipartite (15,15,30)", generateBipartiteGraph(15, 15, 30, 2)},
        {"Cycle (25)", generateCycleGraph(25)},
        {"Path (30)", generatePathGraph(30)}
    };

    for (int i = 0; i < test_graphs.size(); i++) {
        auto [graph_name, graph] = test_graphs[i];

        cout << "\n=== Test " << (i+1) << ": " << graph_name << " ===" << endl;

        // Submit graph
        client.SubmitGraph(graph);
        this_thread::sleep_for(chrono::milliseconds(100));

        // Test various queries
        int max_k = min(10, (int)graph.size() / 2);
        for (int k = 1; k <= max_k; k += 2) {
            client.QueryIndependentSet(k);
            this_thread::sleep_for(chrono::milliseconds(30));
            client.QueryMatching(k);
            this_thread::sleep_for(chrono::milliseconds(30));
        }

        this_thread::sleep_for(chrono::milliseconds(200));
    }

    // Stress test with rapid queries
    cout << "\n=== Stress Test: Rapid Queries ===" << endl;
    auto stress_graph = generateBipartiteGraph(20, 20, 40, 3);
    client.SubmitGraph(stress_graph);
    this_thread::sleep_for(chrono::milliseconds(100));

    for (int i = 0; i < 20; i++) {
        client.QueryIndependentSet(5);
        this_thread::sleep_for(chrono::milliseconds(10));
        client.QueryMatching(5);
        this_thread::sleep_for(chrono::milliseconds(10));
    }

    cout << "\n=== Performance Client 3 Completed ===" << endl;
    return 0;
}
