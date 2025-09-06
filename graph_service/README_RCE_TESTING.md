# Graph Service Testing on RCE Cluster

This guide explains how to test your gRPC graph service on the IIIT RCE cluster with multiple cores.

## Prerequisites

1. **RCE Account Access**: You have been granted access with credentials:
   - Username: cs3401.64
   - Password: X0i7zbFO
   - SSH: `ssh cs3401.64@rce.iiit.ac.in`

2. **Required Files**: Ensure all files are uploaded to the cluster:
   - `server.cpp` - Main server implementation
   - `client.cpp` - Original client
   - `client1.cpp` - Test client 1
   - `client2.cpp` - Test client 2
   - `perf_client1.cpp` - Performance client 1
   - `perf_client2.cpp` - Performance client 2
   - `perf_client3.cpp` - Performance client 3
   - `generate_test_data.cpp` - Test data generator
   - `graph.proto` - Protocol buffer definition
   - `CMakeLists.txt` - Build configuration
   - `test_multi_core.slurm` - Basic multi-core test
   - `performance_test.slurm` - Performance test
   - `run_graph_service.sh` - Original run script

## Setup Instructions

### 1. Connect to RCE Cluster

```bash
ssh cs3401.64@rce.iiit.ac.in
```

### 2. Upload Your Code

Upload all your graph service files to the cluster. You can use `scp` or `rsync`:

```bash
# From your local machine
scp -r /Users/bibeksinghdhody/Downloads/DS/graph_service/* cs3401.64@rce.iiit.ac.in:~/graph_service/
```

### 3. Build the Project

```bash
cd ~/graph_service
mkdir -p build
cd build
cmake ..
make -j4
```

## Running Tests

### Test 1: Basic Multi-Core Test

This test runs the server on one node and two clients on separate nodes to test concurrent graph submissions.

```bash
cd ~/graph_service
sbatch test_multi_core.slurm
```

**What this test does:**
- Allocates 3 nodes (1 for server, 2 for clients)
- Server runs on node 1
- Client 1 runs on node 2 (submits path graphs, random graphs, complete graphs)
- Client 2 runs on node 3 (submits star graphs, cycle graphs, bipartite graphs)
- Tests concurrent graph submissions and queries
- Duration: ~15 minutes

### Test 2: Performance Test

This test runs more intensive performance testing with timing measurements.

```bash
cd ~/graph_service
sbatch performance_test.slurm
```

**What this test does:**
- Allocates 4 nodes (1 for server, 3 for clients)
- Server runs on node 1
- Performance Client 1: Tests with random graphs of increasing sizes
- Performance Client 2: Tests with structured graphs (complete, star)
- Performance Client 3: Tests with mixed graph types and stress testing
- Measures and logs timing for all operations
- Duration: ~30 minutes

## Monitoring Your Jobs

### Check Job Status

```bash
squeue -u cs3401.64
```

### View Job Output

```bash
# View the main log
cat graph_test_<JOB_ID>.log

# View individual client logs
cat client1_<JOB_ID>.log
cat client2_<JOB_ID>.log

# For performance test
cat perf_test_<JOB_ID>.log
cat perf_client1_<JOB_ID>.log
cat perf_client2_<JOB_ID>.log
cat perf_client3_<JOB_ID>.log
```

### Cancel a Job

```bash
scancel <JOB_ID>
```

## Understanding the Test Results

### Basic Multi-Core Test Results

The test verifies:
1. **Concurrent Graph Submissions**: Two clients can submit graphs simultaneously
2. **Graph Union**: Server correctly maintains the union of all client graphs
3. **Query Correctness**: Independent set and matching queries work on combined graphs
4. **Thread Safety**: No race conditions in concurrent access

Expected output patterns:
- `[Client 1]` and `[Client 2]` messages showing concurrent operations
- Server logs showing graph submissions from different clients
- Query results that reflect the combined graph state

### Performance Test Results

The test measures:
1. **Submission Time**: Time to submit graphs of different sizes
2. **Query Time**: Time to process independent set and matching queries
3. **Scalability**: Performance with increasing graph sizes
4. **Concurrent Performance**: How well the system handles multiple clients

Look for timing measurements in microseconds (μs) in the logs.

## Troubleshooting

### Common Issues

1. **Build Failures**:
   - Check if gRPC and protobuf are available: `module avail`
   - Try loading specific versions: `module load gcc/9.3.0 cmake/3.20.0`

2. **Connection Refused**:
   - Server might not have started properly
   - Check server logs for errors
   - Ensure sufficient wait time in scripts

3. **Job Queue Issues**:
   - Check cluster status: `sinfo`
   - Try reducing resource requirements
   - Check if you're within resource limits

4. **Permission Issues**:
   - Ensure files are executable: `chmod +x *.sh`
   - Check file ownership and permissions

### Debug Mode

To run in debug mode, modify the SLURM scripts to use debug nodes:

```bash
#SBATCH --partition=debug
#SBATCH --time=00:05:00
```

## Expected Performance

Based on your implementation:

- **Small graphs** (10-20 vertices): Submissions should complete in <1ms, queries in <10ms
- **Medium graphs** (20-50 vertices): Submissions in 1-5ms, queries in 10-100ms
- **Large graphs** (50+ vertices): Submissions in 5-20ms, queries in 100ms-1s

The exact performance depends on:
- Graph structure (dense vs sparse)
- Query complexity (independent set vs matching)
- Network latency between nodes
- System load

## Next Steps

After running the tests:

1. **Analyze Results**: Check timing measurements and identify bottlenecks
2. **Optimize Code**: If needed, optimize the graph algorithms
3. **Scale Testing**: Try with larger graphs or more clients
4. **Document Findings**: Record performance characteristics for your report

## Additional Resources

- SLURM Documentation: Available online
- gRPC C++ Documentation: https://grpc.io/docs/languages/cpp/
- Protocol Buffers: https://developers.google.com/protocol-buffers

For questions about the cluster or SLURM, post on Moodle or contact the official support channels.
