#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <grpcpp/grpcpp.h>
#include "graph.grpc.pb.h"

using namespace std;
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientWriter;
using grpc::Status;
using graph::GraphProcessor;
using graph::GraphChunk;
using graph::AdjacencyList;
using graph::SubmissionResponse;
using graph::GraphQuery;
using graph::QueryResponse;

class GraphClient {
public:
    GraphClient(shared_ptr<Channel> channel, int client_id)
        : stub_(GraphProcessor::NewStub(channel)), client_id_(client_id) {}

    // Submits a graph to the server in batches of nodes
    void SubmitGraphInChunks(const unordered_map<int, vector<int>>& adj_lists) {
        const int CHUNK_SIZE = 3; // Send 3 nodes per chunk
        SubmissionResponse response;
        ClientContext context;

        unique_ptr<ClientWriter<GraphChunk>> writer(stub_->SubmitGraphStream(&context, &response));

        GraphChunk chunk;
        int nodes_in_current_chunk = 0;

        for (const auto& [node_id, neighbors] : adj_lists) {
            // Add the current node to the chunk
            auto& adj_list = (*chunk.mutable_adjacency_lists_chunk())[node_id];
            for (int neighbor : neighbors) {
                adj_list.add_neighbors(neighbor);
            }
            nodes_in_current_chunk++;

            // If the chunk is full, send it and start a new one
            if (nodes_in_current_chunk >= CHUNK_SIZE) {
                chunk.set_client_id(client_id_);
                cout << "  Sending chunk with " << nodes_in_current_chunk << " nodes..." << endl;
                if (!writer->Write(chunk)) {
                    // Stream has broken
                    break;
                }
                chunk.Clear();
                nodes_in_current_chunk = 0;
            }
        }

        // Send any remaining nodes in the last chunk
        if (nodes_in_current_chunk > 0) {
            chunk.set_client_id(client_id_);
            cout << "  Sending final chunk with " << nodes_in_current_chunk << " nodes..." << endl;
            writer->Write(chunk);
        }

        // Tell the server we're done sending chunks
        writer->WritesDone();
        Status status = writer->Finish();

        if (status.ok()) {
            cout << "Server response: " << response.message() << endl;
        } else {
            cout << "RPC failed: " << status.error_message() << endl;
        }
    }

    // Query for independent set
    void QueryIndependentSet(int k) {
        GraphQuery request;
        request.set_size_threshold(k);
        QueryResponse response;
        ClientContext context;
        Status status = stub_->HasIndependentSet(&context, request, &response);

        if (status.ok()) {
            cout << "Server response: " << response.message() << endl;
            cout << "--> Final Answer: " << (response.result() ? "True" : "False") << endl;
        } else {
            cout << "RPC failed: " << status.error_message() << endl;
        }
    }

    // Query for matching
    void QueryMatching(int k) {
        GraphQuery request;
        request.set_size_threshold(k);
        QueryResponse response;
        ClientContext context;
        Status status = stub_->HasMatching(&context, request, &response);

        if (status.ok()) {
            cout << "Server response: " << response.message() << endl;
            cout << "--> Final Answer: " << (response.result() ? "True" : "False") << endl;
        } else {
            cout << "RPC failed: " << status.error_message() << endl;
        }
    }

private:
    unique_ptr<GraphProcessor::Stub> stub_;
    int client_id_;
};


int main(int argc, char** argv) {
    // Check for a command-line argument for the client ID
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <client_id>" << endl;
        return 1; // Exit with an error
    }
    // Convert the argument from a string to an integer
    int client_id = std::stoi(argv[1]);

    string server_address("localhost:50051");
    // Use the client_id from the command line to create the client
    GraphClient client(grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()), client_id);

    cout << "Interactive gRPC Client (ID: " << client_id << ")" << endl;

    string line;
    while (true) {
        cout << "\n-----------------------" << endl;
        cout << "Commands:" << endl;
        cout << "  submit    - Interactively enter a graph to submit" << endl;
        cout << "  is <k>    - Queries for an independent set of size k" << endl;
        cout << "  match <k> - Queries for a matching of size k" << endl;
        cout << "  quit      - Exits the client" << endl;
        cout << "-----------------------" << endl;

        cout << "> ";
        getline(cin, line);
        stringstream ss(line);
        string command;
        ss >> command;

        if (command == "submit") {
            unordered_map<int, vector<int>> graph_to_send;
            cout << "Enter graph adjacency list (type 'done' on a new line when finished):" << endl;

            while (true) {
                cout << "  Enter node and neighbors (e.g., '0 1 2'): ";
                string graph_line;
                getline(cin, graph_line);
                if (graph_line == "done") {
                    break;
                }
                stringstream gss(graph_line);
                int node_id;
                int neighbor;
                if (gss >> node_id) {
                    graph_to_send[node_id] = {}; // Ensure node exists even if it has no neighbors
                    while (gss >> neighbor) {
                        graph_to_send[node_id].push_back(neighbor);
                    }
                }
            }

            if (!graph_to_send.empty()) {
                cout << "Submitting your graph..." << endl;
                client.SubmitGraphInChunks(graph_to_send);
            } else {
                cout << "No graph entered. Submission cancelled." << endl;
            }
        } else if (command == "is") {
            int k;
            if (ss >> k) {
                client.QueryIndependentSet(k);
            } else {
                cout << "Invalid format. Use: is <k>" << endl;
            }
        } else if (command == "match") {
            int k;
            if (ss >> k) {
                client.QueryMatching(k);
            } else {
                cout << "Invalid format. Use: match <k>" << endl;
            }
        } else if (command == "quit") {
            break;
        } else if (!command.empty()) {
            cout << "Unknown command." << endl;
        }
    }

    return 0;
}