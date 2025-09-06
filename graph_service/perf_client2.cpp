#include <iostream>
#include <memory>
#include <string>
#include <chrono>
#include <thread>
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

// Generate a complete graph
unordered_map<int, vector<int>> generateCompleteGraph(int num_vertices) {
    unordered_map<int, vector<int>> graph;

    // Initialize all vertices
    for (int i = 0; i < num_vertices; i++) {
        graph[i] = vector<int>();
    }

    // Connect every vertex to every other vertex
    for (int i = 0; i < num_vertices; i++) {
        for (int j = i + 1; j < num_vertices; j++) {
            graph[i].push_back(j);
            graph[j].push_back(i);
        }
    }

    return graph;
}

// Generate a star graph
unordered_map<int, vector<int>> generateStarGraph(int num_vertices) {
    unordered_map<int, vector<int>> graph;

    // Initialize all vertices
    for (int i = 0; i < num_vertices; i++) {
        graph[i] = vector<int>();
    }

    // Connect all vertices to vertex 0 (center)
    for (int i = 1; i < num_vertices; i++) {
        graph[0].push_back(i);
        graph[i].push_back(0);
    }

    return graph;
}

int main(int argc, char** argv) {
    cout << "=== Performance Client 2 (Structured Graphs) Starting ===" << endl;

    // Connect to server
    GraphClient client(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()), 2);

    // Wait for server to be ready
    this_thread::sleep_for(chrono::seconds(7));

    // Test with different structured graphs
    vector<int> graph_sizes = {8, 12, 16, 20, 24};

    for (int i = 0; i < graph_sizes.size(); i++) {
        int size = graph_sizes[i];

        cout << "\n=== Test " << (i+1) << ": Complete graph with " << size << " vertices ===" << endl;

        // Generate and submit complete graph
        auto complete_graph = generateCompleteGraph(size);
        client.SubmitGraph(complete_graph);
        this_thread::sleep_for(chrono::milliseconds(100));

        // Test queries on complete graph
        client.QueryIndependentSet(1);  // Only size 1 possible
        this_thread::sleep_for(chrono::milliseconds(50));
        client.QueryMatching(size/2);   // Maximum matching size
        this_thread::sleep_for(chrono::milliseconds(50));

        cout << "\n=== Test " << (i+1) << "b: Star graph with " << size << " vertices ===" << endl;

        // Generate and submit star graph
        auto star_graph = generateStarGraph(size);
        client.SubmitGraph(star_graph);
        this_thread::sleep_for(chrono::milliseconds(100));

        // Test queries on star graph
        client.QueryIndependentSet(size-1);  // All leaves form independent set
        this_thread::sleep_for(chrono::milliseconds(50));
        client.QueryMatching(1);             // Only one edge possible
        this_thread::sleep_for(chrono::milliseconds(50));

        this_thread::sleep_for(chrono::milliseconds(200));
    }

    cout << "\n=== Performance Client 2 Completed ===" << endl;
    return 0;
}
