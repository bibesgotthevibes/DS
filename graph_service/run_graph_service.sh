#!/bin/bash
#SBATCH --job-name=graph_service    # Job name
#SBATCH --nodes=2                   # Number of nodes
#SBATCH --ntasks=2                  # Number of tasks (one for server, one for client)
#SBATCH --cpus-per-task=1          # One CPU per task
#SBATCH --time=00:10:00            # Time limit hrs:min:sec
#SBATCH --output=graph_service_%j.log   # Standard output and error log

# Load any required modules
module purge
module load gcc
module load cmake

# Get the allocated nodes
NODES=($(scontrol show hostname $SLURM_NODELIST))
SERVER_NODE=${NODES[0]}
CLIENT_NODE=${NODES[1]}

# Build the project
cd $SLURM_SUBMIT_DIR
mkdir -p build
cd build
cmake ..
make

# Start the server on the first node
srun --nodes=1 --ntasks=1 -w $SERVER_NODE ./server &
SERVER_PID=$!

# Wait for server to start
sleep 5

# Run the client on the second node
srun --nodes=1 --ntasks=1 -w $CLIENT_NODE ./client

# Clean up
kill $SERVER_PID
