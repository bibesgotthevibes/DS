#!/bin/bash

echo "=== Graph Service Testing on RCE Cluster ==="
echo "This script will guide you through testing your gRPC service"
echo ""

# Step 1: Build the project if necessary
echo "Step 1: Building the project..."
if [ ! -d "build" ]; then
    mkdir -p build
    cd build
    cmake ..
    make -j4
    cd ..
fi
echo "✅ Build completed successfully"
echo ""

# Step 3: Run quick test
echo "Step 3: Running quick test..."
sbatch "${PWD}/quick_test.slurm" # <-- USES FULL PATH
echo "✅ Quick test submitted. Check status with: squeue -u cs3401.64"
echo ""

# Wait for user input
echo "Press Enter to continue to distributed test..."
read

# Step 4: Run distributed test
echo "Step 4: Running distributed test..."
sbatch "${PWD}/test_distributed.slurm" # <-- USES FULL PATH
echo "✅ Distributed test submitted. This tests server on one node, clients on other nodes."
echo ""

# Wait for user input
echo "Press Enter to continue to performance test..."
read

# Step 5: Run performance test
echo "Step 5: Running performance test..."
sbatch "${PWD}/performance_test.slurm" # <-- USES FULL PATH
echo "✅ Performance test submitted. This tests with 3 clients and timing measurements."
echo ""

echo "=== All tests submitted! ==="
echo ""
echo "Monitor your jobs with:"
echo "  squeue -u cs3401.64"