#!/bin/bash
#SBATCH -J mpi_test
#SBATCH -n 2
#SBATCH -N 2
#SBATCH -c 4
#SBATCH -t 00:30:00
#SBATCH -o out
#SBATCH -p batch
unset I_MPI_PMI_LIBRARY
mpiexec.hydra -bootstrap slurm -l \
  -genv KMP_AFFINITY compact \
 ./stencil 200 200 200 20 
