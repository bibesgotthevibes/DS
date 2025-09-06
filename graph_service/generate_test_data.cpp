#include <iostream>
#include <fstream>
#include <random>
#include <vector>
#include <unordered_map>
#include <algorithm>

using namespace std;

class GraphGenerator {
public:
    // Generate a random graph with specified number of vertices and edges
    static unordered_map<int, vector<int>> generateRandomGraph(int num_vertices, int num_edges, int seed = 42) {
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

    // Generate a complete graph
    static unordered_map<int, vector<int>> generateCompleteGraph(int num_vertices) {
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
    static unordered_map<int, vector<int>> generateStarGraph(int num_vertices) {
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
    static unordered_map<int, vector<int>> generateCycleGraph(int num_vertices) {
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
    static unordered_map<int, vector<int>> generatePathGraph(int num_vertices) {
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

    // Generate a bipartite graph
    static unordered_map<int, vector<int>> generateBipartiteGraph(int left_size, int right_size, int num_edges, int seed = 42) {
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

    // Write graph to file in adjacency list format
    static void writeGraphToFile(const unordered_map<int, vector<int>>& graph, const string& filename) {
        ofstream file(filename);
        if (!file.is_open()) {
            cerr << "Error: Could not open file " << filename << endl;
            return;
        }

        file << graph.size() << endl;
        for (const auto& [vertex, neighbors] : graph) {
            file << vertex << " ";
            for (int neighbor : neighbors) {
                file << neighbor << " ";
            }
            file << endl;
        }

        file.close();
        cout << "Graph written to " << filename << endl;
    }

    // Read graph from file
    static unordered_map<int, vector<int>> readGraphFromFile(const string& filename) {
        unordered_map<int, vector<int>> graph;
        ifstream file(filename);

        if (!file.is_open()) {
            cerr << "Error: Could not open file " << filename << endl;
            return graph;
        }

        int num_vertices;
        file >> num_vertices;

        for (int i = 0; i < num_vertices; i++) {
            int vertex;
            file >> vertex;

            vector<int> neighbors;
            int neighbor;
            while (file >> neighbor) {
                neighbors.push_back(neighbor);
                if (file.peek() == '\n' || file.peek() == EOF) break;
            }

            graph[vertex] = neighbors;
        }

        file.close();
        return graph;
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Usage: " << argv[0] << " <graph_type> [parameters...]" << endl;
        cout << "Graph types:" << endl;
        cout << "  random <vertices> <edges> [seed]" << endl;
        cout << "  complete <vertices>" << endl;
        cout << "  star <vertices>" << endl;
        cout << "  cycle <vertices>" << endl;
        cout << "  path <vertices>" << endl;
        cout << "  bipartite <left_size> <right_size> <edges> [seed]" << endl;
        return 1;
    }

    string graph_type = argv[1];
    unordered_map<int, vector<int>> graph;

    if (graph_type == "random") {
        if (argc < 4) {
            cerr << "Usage: random <vertices> <edges> [seed]" << endl;
            return 1;
        }
        int vertices = stoi(argv[2]);
        int edges = stoi(argv[3]);
        int seed = (argc > 4) ? stoi(argv[4]) : 42;
        graph = GraphGenerator::generateRandomGraph(vertices, edges, seed);
    }
    else if (graph_type == "complete") {
        if (argc < 3) {
            cerr << "Usage: complete <vertices>" << endl;
            return 1;
        }
        int vertices = stoi(argv[2]);
        graph = GraphGenerator::generateCompleteGraph(vertices);
    }
    else if (graph_type == "star") {
        if (argc < 3) {
            cerr << "Usage: star <vertices>" << endl;
            return 1;
        }
        int vertices = stoi(argv[2]);
        graph = GraphGenerator::generateStarGraph(vertices);
    }
    else if (graph_type == "cycle") {
        if (argc < 3) {
            cerr << "Usage: cycle <vertices>" << endl;
            return 1;
        }
        int vertices = stoi(argv[2]);
        graph = GraphGenerator::generateCycleGraph(vertices);
    }
    else if (graph_type == "path") {
        if (argc < 3) {
            cerr << "Usage: path <vertices>" << endl;
            return 1;
        }
        int vertices = stoi(argv[2]);
        graph = GraphGenerator::generatePathGraph(vertices);
    }
    else if (graph_type == "bipartite") {
        if (argc < 5) {
            cerr << "Usage: bipartite <left_size> <right_size> <edges> [seed]" << endl;
            return 1;
        }
        int left_size = stoi(argv[2]);
        int right_size = stoi(argv[3]);
        int edges = stoi(argv[4]);
        int seed = (argc > 5) ? stoi(argv[5]) : 42;
        graph = GraphGenerator::generateBipartiteGraph(left_size, right_size, edges, seed);
    }
    else {
        cerr << "Unknown graph type: " << graph_type << endl;
        return 1;
    }

    // Print graph statistics
    int total_edges = 0;
    for (const auto& [vertex, neighbors] : graph) {
        total_edges += neighbors.size();
    }
    total_edges /= 2; // Each edge is counted twice

    cout << "Generated " << graph_type << " graph:" << endl;
    cout << "  Vertices: " << graph.size() << endl;
    cout << "  Edges: " << total_edges << endl;

    // Write to file
    string filename = graph_type + "_graph.txt";
    GraphGenerator::writeGraphToFile(graph, filename);

    return 0;
}
