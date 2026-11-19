
#!/bin/bash
#SBATCH --partition=demo
#SBATCH --time=00:05:00
#SBATCH --exclusive
#SBATCH --output=results/mpi-benchmarks/%x-%j.out

set -euo pipefail

module load openmpi

mode=${1:?Usage: sbatch [Slurm options] mpi-benchmark.slurm seq|mpi1|mpi2|mpi4 SIZE REPETITIONS}
expected_size=${2:?Missing matrix size}
repetitions=${3:?Missing repetition count}

cd "$SLURM_SUBMIT_DIR"

# Validate arguments
[[ "$expected_size" =~ ^[1-9][0-9]*$ && "$repetitions" =~ ^[1-9][0-9]*$ ]] || {
    echo "Size and repetitions must be positive integers" >&2
    exit 1
}

# Select benchmark configuration
case "$mode" in
    seq)
        nodes=1
        ranks=1
        binary_name=sequential_determinant
        ;;
    mpi1)
        nodes=1
        ranks=1
        binary_name=mpi_determinant
        ;;
    mpi2)
        nodes=2
        ranks=2
        binary_name=mpi_determinant
        ;;
    mpi4)
        nodes=2
        ranks=4
        binary_name=mpi_determinant
        ;;
    *)
        echo "Unknown benchmark case: $mode" >&2
        exit 1
        ;;
esac

# Verify Slurm allocation
[[ "$SLURM_JOB_NUM_NODES" == "$nodes" && "$SLURM_NTASKS" == "$ranks" ]] || {
    echo "Slurm allocation does not match the requested case" >&2
    exit 1
}

# Use a separate build directory for each job.
# This prevents simultaneous Slurm jobs from modifying the same build.
build_dir="$SLURM_SUBMIT_DIR/build/mpi-benchmarks-$SLURM_JOB_ID"

echo "========================================"
echo "Case: $mode"
echo "Matrix dimension: $expected_size"
echo "Repetitions: $repetitions"
echo "Slurm job: $SLURM_JOB_ID"
echo "Nodes: $SLURM_JOB_NUM_NODES"
echo "Tasks: $SLURM_NTASKS"
echo "Build directory: $build_dir"
echo "========================================"

echo "Allocated hosts:"
scontrol show hostnames "$SLURM_JOB_NODELIST"

echo "Working directory: $PWD"
echo "Revision: $(git rev-parse HEAD)"

echo "Working tree status:"
git status --short

echo "========================================"
echo "Configuring CMake..."
echo "========================================"

cmake -S . -B "$build_dir" \
    -DCMAKE_BUILD_TYPE=Release \
    -DALGEBRA_ASSIGNMENT_SIZE="$expected_size" \
    -DALGEBRA_ASSIGNMENT_BUILD_MPI=ON \
    -DALGEBRA_ASSIGNMENT_BACKEND=cached \
    -DALGEBRA_ASSIGNMENT_GPROF=OFF

echo "========================================"
echo "Building project..."
echo "========================================"

cmake --build "$build_dir" -j 1

# Select executable
binary="$build_dir/src/$binary_name"

[[ -x "$binary" ]] || {
    echo "Missing executable: $binary" >&2
    exit 1
}

echo "Executable SHA-256:"
sha256sum "$binary"

echo "MPI version:"
mpirun --version | head -n 1

# Prepare execution command
if [[ "$mode" == seq ]]; then
    command=("$binary")
else
    per_node=1
    [[ "$mode" == mpi4 ]] && per_node=2

    command=(
        mpirun
        --display-map
        --report-bindings
        --use-hwthread-cpus
        --map-by "ppr:${per_node}:node"
        -n "$ranks"
        "$binary"
    )
fi

# Execute benchmark repetitions
for ((run=1; run<=repetitions; ++run)); do
    echo
    echo "========================================"
    echo "Run $run of $repetitions"
    date -u '+%Y-%m-%d %H:%M:%S UTC'

    printf 'Command:'
    printf ' %q' "${command[@]}"
    echo

    if "${command[@]}"; then
        echo "Exit status: 0"
    else
        status=$?
        echo "Exit status: $status"
        exit "$status"
    fi
done

echo
echo "All benchmark runs completed successfully."
