#!/bin/bash
#SBATCH --job-name=graph_performance_test
#SBATCH --nodes=4                    # 4 nodes: 1 for server, 3 for clients
#SBATCH --ntasks=4                   # 4 tasks total
#SBATCH --cpus-per-task=4           # 4 CPUs per task for better performance
#SBATCH --time=00:30:00             # 30 minutes time limit
#SBATCH --output=perf_test_%j.log   # Main log file for the job
#SBATCH --error=perf_test_%j.err    # Main error file for the job

# Load the environment needed to RUN the programs
module load gRPC/1.74.1

# Get allocated nodes
NODES=($(scontrol show hostname $SLURM_NODELIST))
SERVER_NODE=${NODES[0]}
CLIENT1_NODE=${NODES[1]}
CLIENT2_NODE=${NODES[2]}
CLIENT3_NODE=${NODES[3]}

echo "=== Graph Service Performance Test Running ==="
echo "Server Node: $SERVER_NODE"
echo "Client Nodes: $CLIENT1_NODE, $CLIENT2_NODE, $CLIENT3_NODE"
echo "Job ID: $SLURM_JOB_ID"

# Generate test data using the pre-built executable
echo "Generating test data..."
./build/generate_test_data random 20 30 1 > /dev/null
./build/generate_test_data complete 15 2 > /dev/null
./build/generate_test_data star 25 3 > /dev/null
./build/generate_test_data bipartite 10 10 20 4 > /dev/null

# Start the server on the first node from the build directory
echo "Starting server on $SERVER_NODE..."
srun --nodes=1 --ntasks=1 -w "$SERVER_NODE" ./build/server > server_perf_${SLURM_JOB_ID}.log 2>&1 &
SERVER_PID=$!

# Wait for server to start
sleep 15

# Check if server is running
if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo "Server failed to start!"
    exit 1
fi

echo "Server started successfully (PID: $SERVER_PID)"

# Run performance test clients from the build directory
echo "Starting performance clients..."
srun --nodes=1 --ntasks=1 -w "$CLIENT1_NODE" ./build/perf_client1 > perf_client1_${SLURM_JOB_ID}.log 2>&1 &
srun --nodes=1 --ntasks=1 -w "$CLIENT2_NODE" ./build/perf_client2 > perf_client2_${SLURM_JOB_ID}.log 2>&1 &
srun --nodes=1 --ntasks=1 -w "$CLIENT3_NODE" ./build/perf_client3 > perf_client3_${SLURM_JOB_ID}.log 2>&1 &

# Wait for all clients to complete
echo "Waiting for all clients to complete..."
wait

# Clean up server
echo "Stopping server..."
kill $SERVER_PID
wait $SERVER_PID 2>/dev/null

echo "Performance test completed."