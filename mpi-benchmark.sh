#!/usr/bin/env bash

set -euo pipefail

if [[ $# -ne 1 || ! $1 =~ ^[1-9][0-9]*$ ]]; then
  echo "Usage: $0 <matrix-size>" >&2
  echo "Example: $0 256" >&2
  exit 1
fi

n="$1"

# Run from the directory containing this script.
cd -- "$(dirname -- "${BASH_SOURCE[0]}")"

# Add a unique suffix so repeated invocations cannot overwrite earlier logs.
results_dir="results/mpi-benchmarks"
mkdir -p "$results_dir"
bench_dir=$(mktemp -d "$results_dir/${HOSTNAME}-$(date -u +%Y%m%dT%H%M%SZ)-XXXXXX")

echo "Configuring for matrix size $n..."
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DALGEBRA_ASSIGNMENT_SIZE="$n" \
  -DALGEBRA_ASSIGNMENT_BUILD_MPI=ON \
  -DALGEBRA_ASSIGNMENT_BACKEND=cached

echo "Building..."
cmake --build build --parallel

echo "Running sequential benchmarks..."
for run in 1 2 3; do
  echo "Sequential run $run"
  ./build/src/sequential_determinant \
    2>&1 | tee "$bench_dir/seq-n${n}-run${run}.log"
done

for processes in 1 2 4; do
  for run in 1 2 3; do
    echo "MPI run $run with $processes process(es)"
    mpirun \
      --display-map \
      --report-bindings \
      --use-hwthread-cpus \
      -n "$processes" \
      ./build/src/mpi_determinant \
      2>&1 | tee "$bench_dir/mpi${processes}-n${n}-run${run}.log"
  done
done

echo "Logs saved in $bench_dir"