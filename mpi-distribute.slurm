#!/bin/bash
#SBATCH --job-name=mpi-distribute
#SBATCH --partition=demo
#SBATCH --nodes=2
#SBATCH --ntasks=3
#SBATCH --ntasks-per-node=2
#SBATCH --time=00:02:00
#SBATCH --output=mpi-distribute-%j.out

module load openmpi

cd "$SLURM_SUBMIT_DIR"
mpirun -n 3 ./build/src/mpi_distribute