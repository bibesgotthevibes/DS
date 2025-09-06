#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include <mutex>
#include <unordered_map>
#include <vector>
#include "graph.grpc.pb.h"

using namespace std;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using graph::GraphProcessor;
using graph::Graph;
using graph::SubmissionResponse;
using graph::IndependentSetQuery;
using graph::MatchingQuery;
using graph::QueryResponse;

class GraphProcessorImpl final : public GraphProcessor::Service {
private:
    mutex graph_mutex;
    unordered_map<int, unordered_map<int, vector<int>>> client_graphs;

    // Helper function to find maximal independent set
    bool hasIndependentSetOfSize(const unordered_map<int, vector<int>>& combined_graph, int k) {
        vector<int> vertices;
        for (const auto& pair : combined_graph) {
            vertices.push_back(pair.first);
        }

        std::vector<int> current_set;
        return findIndependentSet(combined_graph, vertices, 0, k, current_set);
    }

    bool findIndependentSet(const unordered_map<int, vector<int>>& graph,
                          const vector<int>& vertices,
                          int index, int k,
                          vector<int>& current_set) {
        if (current_set.size() >= k) return true;
        if (index >= vertices.size()) return false;

        // Try excluding current vertex
        if (findIndependentSet(graph, vertices, index + 1, k, current_set)) {
            return true;
        }

        // Check if we can add current vertex
        int current_vertex = vertices[index];
        bool can_add = true;
        for (int vertex : current_set) {
            auto it = graph.find(vertex);
            if (it != graph.end() &&
                std::find(it->second.begin(), it->second.end(), current_vertex) != it->second.end()) {
                can_add = false;
                break;
            }
        }

        if (can_add) {
            current_set.push_back(current_vertex);
            if (findIndependentSet(graph, vertices, index + 1, k, current_set)) {
                return true;
            }
            current_set.pop_back();
        }

        return false;
    }

    // Helper function to find matching
    bool hasMatchingOfSize(const unordered_map<int, vector<int>>& combined_graph, int k) {
        vector<pair<int, int>> edges;
        unordered_map<int, vector<int>> matching;

        // Create list of edges
        for (const auto& [vertex, neighbors] : combined_graph) {
            for (int neighbor : neighbors) {
                if (vertex < neighbor) { // Add each edge only once
                    edges.emplace_back(vertex, neighbor);
                }
            }
        }
        return findMatching(edges, 0, k, matching);
    }

    bool findMatching(const vector<pair<int, int>>& edges,
                     int index, int k,
                     unordered_map<int, vector<int>>& matching) {
        if (matching.size() >= k) return true;
        if (index >= edges.size()) return false;

        // Try excluding current edge
        if (findMatching(edges, index + 1, k, matching)) {
            return true;
        }

        // Try including current edge if possible
        auto [v1, v2] = edges[index];
        if (matching.find(v1) == matching.end() && matching.find(v2) == matching.end()) {
            matching[v1].push_back(v2);
            matching[v2].push_back(v1);
            if (findMatching(edges, index + 1, k, matching)) {
                return true;
            }
            matching.erase(v1);
            matching.erase(v2);
        }

        return false;
    }

public:
    Status SubmitGraph(ServerContext* context, const Graph* request, SubmissionResponse* response) override {
        std::lock_guard<std::mutex> lock(graph_mutex);

        int client_id = request->client_id();
        cout << "Received graph from client " << client_id << endl;
        client_graphs[client_id].clear();        for (const auto& [vertex, adj_list] : request->adjacency_lists()) {
            client_graphs[client_id][vertex] = std::vector<int>(
                adj_list.neighbors().begin(),
                adj_list.neighbors().end()
            );
        }

        response->set_success(true);
        cout << "Graph from client " << client_id << " has " << request->adjacency_lists().size() << " vertices" << endl;
        response->set_message("Graph successfully submitted");
        return Status::OK;
    }

    Status HasIndependentSet(ServerContext* context, const IndependentSetQuery* request,
                           QueryResponse* response) override {
        std::lock_guard<std::mutex> lock(graph_mutex);

        // Combine graphs
        std::unordered_map<int, std::vector<int>> combined_graph;
        for (const auto& [client_id, graph] : client_graphs) {
            for (const auto& [vertex, neighbors] : graph) {
                combined_graph[vertex].insert(
                    combined_graph[vertex].end(),
                    neighbors.begin(),
                    neighbors.end()
                );
            }
        }

        bool has_independent_set = hasIndependentSetOfSize(combined_graph, request->size_threshold());

        response->set_result(has_independent_set);
        response->set_message(has_independent_set ?
            "Found independent set of required size" :
            "No independent set of required size exists");
        return Status::OK;
    }

    Status HasMatching(ServerContext* context, const MatchingQuery* request,
                      QueryResponse* response) override {
        std::lock_guard<std::mutex> lock(graph_mutex);

        // Combine graphs
        std::unordered_map<int, std::vector<int>> combined_graph;
        for (const auto& [client_id, graph] : client_graphs) {
            for (const auto& [vertex, neighbors] : graph) {
                combined_graph[vertex].insert(
                    combined_graph[vertex].end(),
                    neighbors.begin(),
                    neighbors.end()
                );
            }
        }

        bool has_matching = hasMatchingOfSize(combined_graph, request->size_threshold());

        response->set_result(has_matching);
        response->set_message(has_matching ?
            "Found matching of required size" :
            "No matching of required size exists");
        return Status::OK;
    }
};

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
