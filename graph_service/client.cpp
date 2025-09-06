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

void sleep_for_seconds(int seconds) {
    this_thread::sleep_for(chrono::seconds(seconds));
}

class GraphClient {
public:
    GraphClient(shared_ptr<Channel> channel)
        : stub_(GraphProcessor::NewStub(channel)) {}

    // Submit a graph to the server
    bool SubmitGraph(int client_id, const unordered_map<int, vector<int>>& adj_lists) {
        Graph request;
        request.set_client_id(client_id);

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
            cout << "Graph submitted successfully: " << response.message() << endl;
            return response.success();
        } else {
            cout << "Error submitting graph: " << status.error_message() << endl;
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
            cout << "Independent set query result: " << response.message() << endl;
            return response.result();
        } else {
            cout << "Error querying independent set: " << status.error_message() << endl;
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
            cout << "Matching query result: " << response.message() << endl;
            return response.result();
        } else {
            cout << "Error querying matching: " << status.error_message() << endl;
            return false;
        }
    }

private:
    unique_ptr<GraphProcessor::Stub> stub_;
};

int main(int argc, char** argv) {
    // Create two clients connected to the server
    GraphClient client1(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));
    GraphClient client2(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));

    cout << "\n=== Test Case 1: Query before any graphs are submitted ===" << endl;
    sleep_for_seconds(1);
    bool has_independent_set = client1.QueryIndependentSet(1);
    cout << "Query for independent set of size 1 (empty graph): " << (has_independent_set ? "Yes" : "No") << endl;
    sleep_for_seconds(1);
    bool has_matching = client1.QueryMatching(1);
    cout << "Query for matching of size 1 (empty graph): " << (has_matching ? "Yes" : "No") << endl;
    sleep_for_seconds(1);

    cout << "\n=== Test Case 2: Submit and query path graph from client 1 ===" << endl;
    // Path graph: 0 -- 1 -- 2 -- 3
    unordered_map<int, vector<int>> path_graph = {
        {0, {1}},
        {1, {0, 2}},
        {2, {1, 3}},
        {3, {2}}
    };
    cout << "Submitting path graph from client 1..." << endl;
    client1.SubmitGraph(1, path_graph);
    sleep_for_seconds(1);

    has_independent_set = client1.QueryIndependentSet(2);
    cout << "Has independent set of size 2 (path graph): " << (has_independent_set ? "Yes" : "No") << endl;
    sleep_for_seconds(1);
    has_matching = client1.QueryMatching(2);
    cout << "Has matching of size 2 (path graph): " << (has_matching ? "Yes" : "No") << endl;
    sleep_for_seconds(1);

    cout << "\n=== Test Case 3: Submit star graph from client 2 ===" << endl;
    // Star graph: Center vertex 0 connected to vertices 1,2,3,4
    unordered_map<int, vector<int>> star_graph = {
        {0, {1, 2, 3, 4}},
        {1, {0}},
        {2, {0}},
        {3, {0}},
        {4, {0}}
    };
    cout << "Submitting star graph from client 2..." << endl;
    client2.SubmitGraph(2, star_graph);
    sleep_for_seconds(1);

    has_independent_set = client2.QueryIndependentSet(4);
    cout << "Has independent set of size 4 (combined graphs): " << (has_independent_set ? "Yes" : "No") << endl;
    sleep_for_seconds(1);
    has_matching = client2.QueryMatching(3);
    cout << "Has matching of size 3 (combined graphs): " << (has_matching ? "Yes" : "No") << endl;
    sleep_for_seconds(1);

    cout << "\n=== Test Case 4: Update client 1's graph to be a complete graph ===" << endl;
    // Complete graph on 4 vertices
    unordered_map<int, vector<int>> complete_graph = {
        {0, {1, 2, 3}},
        {1, {0, 2, 3}},
        {2, {0, 1, 3}},
        {3, {0, 1, 2}}
    };
    cout << "Updating client 1's graph to complete graph..." << endl;
    client1.SubmitGraph(1, complete_graph);
    sleep_for_seconds(1);

    has_independent_set = client1.QueryIndependentSet(2);
    cout << "Has independent set of size 2 (combined complete + star): " << (has_independent_set ? "Yes" : "No") << endl;
    sleep_for_seconds(1);
    has_matching = client1.QueryMatching(4);
    cout << "Has matching of size 4 (combined complete + star): " << (has_matching ? "Yes" : "No") << endl;
    sleep_for_seconds(1);

    cout << "\n=== Test Case 5: Large independent set query ===" << endl;
    has_independent_set = client1.QueryIndependentSet(5);
    cout << "Has independent set of size 5 (should be false): " << (has_independent_set ? "Yes" : "No") << endl;

    return 0;
}
