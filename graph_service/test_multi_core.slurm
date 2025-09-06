#!/bin/bash
#SBATCH --job-name=graph_multi_core_test
#SBATCH --nodes=3                    # 3 nodes: 1 for server, 2 for clients
#SBATCH --ntasks=3                   # 3 tasks total
#SBATCH --cpus-per-task=2           # 2 CPUs per task for better performance
#SBATCH --time=00:15:00             # 15 minutes time limit
#SBATCH --output=graph_test_%j.log  # Main log file for the job
#SBATCH --error=graph_test_%j.err   # Main error file for the job

# Load the environment needed to RUN the programs
module load gRPC/1.74.1

# Get allocated nodes
NODES=($(scontrol show hostname $SLURM_NODELIST))
SERVER_NODE=${NODES[0]}
CLIENT1_NODE=${NODES[1]}
CLIENT2_NODE=${NODES[2]}

echo "=== Graph Service Multi-Core Test Running ==="
echo "Server Node: $SERVER_NODE"
echo "Client 1 Node: $CLIENT1_NODE"
echo "Client 2 Node: $CLIENT2_NODE"
echo "Job ID: $SLURM_JOB_ID"

# Start the server on the first node from the build directory
echo "Starting server on $SERVER_NODE..."
srun --nodes=1 --ntasks=1 -w "$SERVER_NODE" ./build/server > server_${SLURM_JOB_ID}.log 2>&1 &
SERVER_PID=$!

# Wait for server to start
sleep 15

# Check if server is running
if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo "Server failed to start!"
    exit 1
fi

echo "Server started successfully (PID: $SERVER_PID)"

# Run client 1 on the second node from the build directory
echo "Starting client 1 on $CLIENT1_NODE..."
srun --nodes=1 --ntasks=1 -w "$CLIENT1_NODE" ./build/client1 > client1_${SLURM_JOB_ID}.log 2>&1 &
CLIENT1_PID=$!

# Run client 2 on the third node from the build directory
echo "Starting client 2 on $CLIENT2_NODE..."
srun --nodes=1 --ntasks=1 -w "$CLIENT2_NODE" ./build/client2 > client2_${SLURM_JOB_ID}.log 2>&1 &
CLIENT2_PID=$!

# Wait for clients to complete
echo "Waiting for clients to complete..."
wait $CLIENT1_PID
wait $CLIENT2_PID

# Clean up server
echo "Stopping server..."
kill $SERVER_PID
wait $SERVER_PID 2>/dev/null

echo "Test completed."