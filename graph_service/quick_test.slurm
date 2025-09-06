#!/bin/bash
#SBATCH --job-name=graph_quick_test
#SBATCH --nodes=2                    # 2 nodes: 1 for server, 1 for client
#SBATCH --ntasks=2                   # 2 tasks total
#SBATCH --cpus-per-task=1           # 1 CPU per task
#SBATCH --time=00:05:00             # 5 minutes time limit
#SBATCH --output=quick_test_%j.log  # Main log file for the job
#SBATCH --error=quick_test_%j.err   # Main error file for the job

# Load the environment needed to RUN the programs
module load gRPC/1.74.1

# Get allocated nodes
NODES=($(scontrol show hostname $SLURM_NODELIST))
SERVER_NODE=${NODES[0]}
CLIENT_NODE=${NODES[1]}

echo "=== Graph Service Quick Test Running ==="
echo "Server Node: $SERVER_NODE"
echo "Client Node: $CLIENT_NODE"
echo "Job ID: $SLURM_JOB_ID"

# Start the server on the first node from the build directory
echo "Starting server on $SERVER_NODE..."
srun --nodes=1 --ntasks=1 -w "$SERVER_NODE" ./build/server > server_quick_${SLURM_JOB_ID}.log 2>&1 &
SERVER_PID=$!

# Wait for server to start
sleep 5

# Check if server is running
if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo "Server failed to start!"
    exit 1
fi

echo "Server started successfully (PID: $SERVER_PID)"

# Run the client on the second node from the build directory
echo "Running client on $CLIENT_NODE..."
srun --nodes=1 --ntasks=1 -w "$CLIENT_NODE" ./build/client > client_quick_${SLURM_JOB_ID}.log 2>&1
CLIENT_EXIT=$?

echo "Client exit code: $CLIENT_EXIT"

# Clean up server
echo "Stopping server..."
kill $SERVER_PID
wait $SERVER_PID 2>/dev/null

echo "Quick test completed at: $(date)"