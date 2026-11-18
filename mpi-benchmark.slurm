#!/bin/bash
#SBATCH --partition=demo
#SBATCH --time=00:05:00
#SBATCH --exclusive
#SBATCH --output=results/mpi-benchmarks/%x-%j.out

set -eo pipefail
module load openmpi
set -u

mode=${1:?Usage: sbatch [Slurm options] mpi-benchmark.slurm seq|mpi1|mpi2|mpi4 SIZE REPETITIONS}
expected_size=${2:?Missing matrix size}
repetitions=${3:?Missing repetition count}
cd "$SLURM_SUBMIT_DIR"

case "$mode" in
    seq)  nodes=1; ranks=1; binary=./build/src/sequential_determinant ;;
    mpi1) nodes=1; ranks=1; binary=./build/src/mpi_determinant ;;
    mpi2) nodes=2; ranks=2; binary=./build/src/mpi_determinant ;;
    mpi4) nodes=2; ranks=4; binary=./build/src/mpi_determinant ;;
    *) echo "Unknown benchmark case: $mode" >&2; exit 1 ;;
esac
[[ "$expected_size" =~ ^[1-9][0-9]*$ && "$repetitions" =~ ^[1-9][0-9]*$ ]] || { echo 'Size and repetitions must be positive integers' >&2; exit 1; }
[[ "$SLURM_JOB_NUM_NODES" == "$nodes" && "$SLURM_NTASKS" == "$ranks" ]] || { echo 'Slurm allocation does not match the requested case' >&2; exit 1; }

cache=build/CMakeCache.txt
for setting in "CMAKE_BUILD_TYPE:STRING=Release" "ALGEBRA_ASSIGNMENT_BACKEND:STRING=cached" "ALGEBRA_ASSIGNMENT_SIZE:STRING=$expected_size"; do
    grep -Fxq "$setting" "$cache" || { echo "Unexpected build setting: expected $setting" >&2; exit 1; }
done
grep -Eq '^ALGEBRA_ASSIGNMENT_BUILD_MPI:BOOL=(ON|TRUE|YES|1)$' "$cache" || { echo 'MPI must be enabled in the build' >&2; exit 1; }
grep -Eq '^ALGEBRA_ASSIGNMENT_GPROF:BOOL=(OFF|FALSE|NO|0)$' "$cache" || { echo 'Profiling must be disabled for benchmark runs' >&2; exit 1; }
[[ -x "$binary" ]] || { echo "Missing executable: $binary" >&2; exit 1; }

echo "Case: $mode; expected dimension: $expected_size; repetitions: $repetitions"
echo "Slurm job: $SLURM_JOB_ID; nodes: $SLURM_JOB_NUM_NODES; tasks: $SLURM_NTASKS"
echo "Allocated hosts:"
scontrol show hostnames "$SLURM_JOB_NODELIST"
echo "Working directory: $PWD"
echo "Revision: $(git rev-parse HEAD)"
echo 'Working tree status:'
git status --short
echo 'Executable SHA-256:'
sha256sum "$binary"
echo 'Build settings:'
grep -E '^(CMAKE_BUILD_TYPE|ALGEBRA_ASSIGNMENT_(BACKEND|SIZE|BUILD_MPI|GPROF)):' "$cache"
mpirun --version | head -n 1

if [[ "$mode" == seq ]]; then
    command=("$binary")
else
    per_node=1
    [[ "$mode" == mpi4 ]] && per_node=2
    command=(mpirun --display-map --report-bindings --use-hwthread-cpus --map-by "ppr:${per_node}:node" -n "$ranks" "$binary")
fi

for ((run=1; run<=repetitions; ++run)); do
    echo
    echo "Run $run of $repetitions; $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
    printf 'Command:'
    printf ' %q' "${command[@]}"
    echo
    if "${command[@]}"; then
        echo 'Exit status: 0'
    else
        status=$?
        echo "Exit status: $status"
        exit "$status"
    fi
done
