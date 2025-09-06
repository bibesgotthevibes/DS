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
    cout << "=== Client 1 Starting ===" << endl;

    // --- IMPORTANT FIX: Get server hostname from environment ---
    // This allows the client to find the server when running on the cluster
    string server_host = "localhost";
    if (const char* env_host = getenv("SERVER_HOST")) {
        server_host = env_host;
    }
    string server_address = server_host + ":50051";
    cout << "Connecting to server at: " << server_address << endl;
    // --- END FIX ---

    // Connect to server
    GraphClient client(grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()), 1);

    // Wait a bit for server to be ready
    this_thread::sleep_for(chrono::seconds(2));

    cout << "\n=== Test 1: Submit path graph ===" << endl;
    unordered_map<int, vector<int>> path_graph = {
        {0, {1}},
        {1, {0, 2}},
        {2, {1, 3}},
        {3, {2}}
    };
    client.SubmitGraph(path_graph);
    this_thread::sleep_for(chrono::milliseconds(500));

    cout << "\n=== Test 2: Query independent set ===" << endl;
    bool result_is_2 = client.QueryIndependentSet(2);
    cout << "--> Final Answer: " << (result_is_2 ? "True" : "False") << endl; // Added this
    this_thread::sleep_for(chrono::milliseconds(500));

    cout << "\n=== Test 3: Query matching ===" << endl;
    bool result_m_2 = client.QueryMatching(2);
    cout << "--> Final Answer: " << (result_m_2 ? "True" : "False") << endl; // Added this
    this_thread::sleep_for(chrono::milliseconds(500));

    cout << "\n=== Test 4: Submit larger random graph ===" << endl;
    auto random_graph = generateRandomGraph(10, 15, 42);
    client.SubmitGraph(random_graph);
    this_thread::sleep_for(chrono::milliseconds(500));

    cout << "\n=== Test 5: Query on combined graph ===" << endl;
    bool result_is_5 = client.QueryIndependentSet(5);
    cout << "--> Final Answer (IS): " << (result_is_5 ? "True" : "False") << endl; // Added this
    this_thread::sleep_for(chrono::milliseconds(500));
    bool result_m_4 = client.QueryMatching(4);
    cout << "--> Final Answer (Matching): " << (result_m_4 ? "True" : "False") << endl; // Added this
    this_thread::sleep_for(chrono::milliseconds(500));

    cout << "\n=== Test 6: Submit complete graph ===" << endl;
    unordered_map<int, vector<int>> complete_graph = {
        {0, {1, 2, 3}},
        {1, {0, 2, 3}},
        {2, {0, 1, 3}},
        {3, {0, 1, 2}}
    };
    client.SubmitGraph(complete_graph);
    this_thread::sleep_for(chrono::milliseconds(500));

    cout << "\n=== Test 7: Final queries ===" << endl;
    bool result_is_1 = client.QueryIndependentSet(1);
    cout << "--> Final Answer (IS): " << (result_is_1 ? "True" : "False") << endl; // Added this
    this_thread::sleep_for(chrono::milliseconds(500));
    bool result_m_2_final = client.QueryMatching(2);
    cout << "--> Final Answer (Matching): " << (result_m_2_final ? "True" : "False") << endl; // Added this

    cout << "\n=== Client 1 Completed ===" << endl;
    return 0;
}
