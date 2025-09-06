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

        if (status.ok()) {
            cout << "[Client " << client_id_ << "] Graph submitted: " << response.message() << endl;
            return response.success();
        } else {
            cout << "[Client " << client_id_ << "] Error submitting graph: " << status.error_message() << endl;
            return false;
        }
    }

    // Query if there exists an independent set of size k
    bool QueryIndependentSet(int k) {
        IndependentSetQuery request;
        request.set_size_threshold(k);

        QueryResponse response;
        ClientContext context;

        Status status = stub_->HasIndependentSet(&context, request, &response);

        if (status.ok()) {
            cout << "[Client " << client_id_ << "] Independent set query (k=" << k << "): " << response.message() << endl;
            return response.result();
        } else {
            cout << "[Client " << client_id_ << "] Error querying independent set: " << status.error_message() << endl;
            return false;
        }
    }

    // Query if there exists a matching of size k
    bool QueryMatching(int k) {
        MatchingQuery request;
        request.set_size_threshold(k);

        QueryResponse response;
        ClientContext context;

        Status status = stub_->HasMatching(&context, request, &response);

        if (status.ok()) {
            cout << "[Client " << client_id_ << "] Matching query (k=" << k << "): " << response.message() << endl;
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

int main(int argc, char** argv) {
    cout << "=== Client 2 Starting (Distributed) ===" << endl;

    // Get server hostname from environment or use default
    string server_host = "localhost";
    if (const char* env_host = getenv("SERVER_HOST")) {
        server_host = env_host;
    }

    string server_address = server_host + ":50051";
    cout << "Connecting to server at: " << server_address << endl;

    // Connect to server
    GraphClient client(grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()), 2);

    // Wait a bit for server to be ready
    this_thread::sleep_for(chrono::seconds(3));

    cout << "\n=== Test 1: Submit star graph ===" << endl;
    auto star_graph = generateStarGraph(6);
    client.SubmitGraph(star_graph);
    this_thread::sleep_for(chrono::milliseconds(500));

    cout << "\n=== Test 2: Query independent set ===" << endl;
    bool result_is_2 = client.QueryIndependentSet(5);
    cout << "--> Final Answer: " << (result_is_2 ? "True" : "False") << endl; // Added this
    this_thread::sleep_for(chrono::milliseconds(500));

    cout << "\n=== Test 3: Query matching ===" << endl;
    bool result_m_3 = client.QueryMatching(3);
    cout << "--> Final Answer: " << (result_m_3 ? "True" : "False") << endl; // Added this
    this_thread::sleep_for(chrono::milliseconds(500));

    cout << "\n=== Test 4: Submit cycle graph ===" << endl;
    auto cycle_graph = generateCycleGraph(8);
    client.SubmitGraph(cycle_graph);
    this_thread::sleep_for(chrono::milliseconds(500));

    cout << "\n=== Test 5: Query on combined graph ===" << endl;
    bool result_is_4 = client.QueryIndependentSet(4);
    cout << "--> Final Answer (IS): " << (result_is_4 ? "True" : "False") << endl; // Added this
    this_thread::sleep_for(chrono::milliseconds(500));
    bool result_m_4 = client.QueryMatching(4);
    cout << "--> Final Answer (Matching): " << (result_m_4 ? "True" : "False") << endl; // Added this
    this_thread::sleep_for(chrono::milliseconds(500));

    cout << "\n=== Test 6: Submit bipartite graph ===" << endl;
    unordered_map<int, vector<int>> bipartite_graph = {
        {0, {4, 5, 6}}, {1, {4, 5, 6}}, {2, {4, 5, 6}}, {3, {4, 5, 6}},
        {4, {0, 1, 2, 3}}, {5, {0, 1, 2, 3}}, {6, {0, 1, 2, 3}}
    };
    client.SubmitGraph(bipartite_graph);
    this_thread::sleep_for(chrono::milliseconds(500));

    cout << "\n=== Test 7: Final queries ===" << endl;
    bool result_is_7 = client.QueryIndependentSet(7);
    cout << "--> Final Answer (IS): " << (result_is_7 ? "True" : "False") << endl; // Added this
    this_thread::sleep_for(chrono::milliseconds(500));
    bool result_m_3_final = client.QueryMatching(3);
    cout << "--> Final Answer (Matching): " << (result_m_3_final ? "True" : "False") << endl; // Added this
    this_thread::sleep_for(chrono::milliseconds(500));

    cout << "\n=== Test 8: Stress test with large queries ===" << endl;
    bool result_is_10 = client.QueryIndependentSet(10);
    cout << "--> Final Answer (IS): " << (result_is_10 ? "True" : "False") << endl; // Added this
    this_thread::sleep_for(chrono::milliseconds(500));
    bool result_m_8 = client.QueryMatching(8);
    cout << "--> Final Answer (Matching): " << (result_m_8 ? "True" : "False") << endl; // Added this

    cout << "\n=== Client 2 Completed ===" << endl;
    return 0;
}