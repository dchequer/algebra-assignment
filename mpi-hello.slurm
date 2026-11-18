#!/bin/bash
#SBATCH --job-name=mpi-hello
#SBATCH --partition=demo
#SBATCH --nodes=2
#SBATCH --ntasks=4
#SBATCH --ntasks-per-node=2
#SBATCH --time=00:02:00
#SBATCH --output=mpi-hello-%j.out

module load openmpi

cd "$SLURM_SUBMIT_DIR"
mpirun -n 4 ./build/src/mpi_hello