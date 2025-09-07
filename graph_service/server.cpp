#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <grpcpp/grpcpp.h>
#include <mutex>
#include <unordered_map>
#include "graph.grpc.pb.h"

using namespace std;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::Status;
using graph::GraphProcessor;
using graph::GraphChunk;
using graph::AdjacencyList;
using graph::SubmissionResponse;
using graph::GraphQuery;
using graph::QueryResponse;

class GraphProcessorImpl final : public GraphProcessor::Service {
private:
    mutex graph_mutex;
    unordered_map<int, unordered_map<int, vector<int>>> client_graphs;

    // --- ALGORITHMS (CORRECTED VERSIONS) ---

    bool isSafe(const unordered_map<int, vector<int>>& graph, int vertex, const vector<int>& current_set) {
        for (int v_in_set : current_set) {
            auto it = graph.find(v_in_set);
            if (it != graph.end()) {
                for (int neighbor : it->second) {
                    if (neighbor == vertex) return false;
                }
            }
        }
        return true;
    }

    bool findIndependentSetRecursive(const unordered_map<int, vector<int>>& graph, const vector<int>& vertices, int index, int k, vector<int>& current_set) {
        if (current_set.size() >= k) return true;
        if (index >= vertices.size()) return false;
        int current_vertex = vertices[index];
        if (isSafe(graph, current_vertex, current_set)) {
            current_set.push_back(current_vertex);
            if (findIndependentSetRecursive(graph, vertices, index + 1, k, current_set)) return true;
            current_set.pop_back();
        }
        if (findIndependentSetRecursive(graph, vertices, index + 1, k, current_set)) return true;
        return false;
    }

    bool hasIndependentSetOfSize(const unordered_map<int, vector<int>>& combined_graph, int k) {
        vector<int> vertices;
        for (const auto& pair : combined_graph) vertices.push_back(pair.first);
        vector<int> current_set;
        return findIndependentSetRecursive(combined_graph, vertices, 0, k, current_set);
    }

    bool findMatchingRecursive(const vector<pair<int, int>>& edges, int index, int k, unordered_map<int, bool>& matched_vertices) {
        if (matched_vertices.size() / 2 >= k) return true;
        if (index >= edges.size()) return false;
        if (findMatchingRecursive(edges, index + 1, k, matched_vertices)) return true;
        auto [v1, v2] = edges[index];
        if (matched_vertices.find(v1) == matched_vertices.end() && matched_vertices.find(v2) == matched_vertices.end()) {
            matched_vertices[v1] = true;
            matched_vertices[v2] = true;
            if (findMatchingRecursive(edges, index + 1, k, matched_vertices)) return true;
            matched_vertices.erase(v1);
            matched_vertices.erase(v2);
        }
        return false;
    }

    bool hasMatchingOfSize(const unordered_map<int, vector<int>>& combined_graph, int k) {
        vector<pair<int, int>> edges;
        for (const auto& [vertex, neighbors] : combined_graph) {
            for (int neighbor : neighbors) {
                if (vertex < neighbor) edges.emplace_back(vertex, neighbor);
            }
        }
        unordered_map<int, bool> matched_vertices;
        return findMatchingRecursive(edges, 0, k, matched_vertices);
    }

    // --- HELPER FOR GRAPH UNION ---

    unordered_map<int, vector<int>> getCombinedGraph() {
        unordered_map<int, vector<int>> combined_graph;
        for (const auto& [client_id, graph] : client_graphs) {
            for (const auto& [vertex, neighbors] : graph) {
                combined_graph[vertex].insert(combined_graph[vertex].end(), neighbors.begin(), neighbors.end());
            }
        }
        return combined_graph;
    }

public:
    // --- RPC IMPLEMENTATIONS ---

    Status SubmitGraphStream(ServerContext* context, ServerReader<GraphChunk>* reader, SubmissionResponse* response) override {
        lock_guard<mutex> lock(graph_mutex);
        GraphChunk chunk;
        int client_id = -1;
        bool first_chunk = true;

        while (reader->Read(&chunk)) {
            if (first_chunk) {
                client_id = chunk.client_id();
                cout << "Receiving graph from client " << client_id << "..." << endl;
                client_graphs[client_id].clear();
                first_chunk = false;
            }
            for (const auto& [node_id, adj_list] : chunk.adjacency_lists_chunk()) {
                client_graphs[client_id][node_id].assign(adj_list.neighbors().begin(), adj_list.neighbors().end());
            }
        }
        cout << "Stream from client " << client_id << " finished." << endl;
        response->set_success(true);
        response->set_message("Graph successfully received and reassembled.");
        return Status::OK;
    }

    Status HasIndependentSet(ServerContext* context, const GraphQuery* request, QueryResponse* response) override {
        lock_guard<mutex> lock(graph_mutex);
        auto combined_graph = getCombinedGraph();
        bool result = hasIndependentSetOfSize(combined_graph, request->size_threshold());
        response->set_result(result);
        response->set_message(result ? "Found independent set of required size" : "No independent set of required size exists");
        return Status::OK;
    }

    Status HasMatching(ServerContext* context, const GraphQuery* request, QueryResponse* response) override {
        lock_guard<mutex> lock(graph_mutex);
        auto combined_graph = getCombinedGraph();
        bool result = hasMatchingOfSize(combined_graph, request->size_threshold());
        response->set_result(result);
        response->set_message(result ? "Found matching of required size" : "No matching of required size exists");
        return Status::OK;
    }
};

// --- SERVER STARTUP ---
void RunServer() {
    string server_address("0.0.0.0:50051");
    GraphProcessorImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    unique_ptr<Server> server(builder.BuildAndStart());
    cout << "Server listening on " << server_address << endl;
    server->Wait();
}

int main(int argc, char** argv) {
    RunServer();
    return 0;
}