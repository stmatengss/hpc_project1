#!/bin/bash
#SBATCH -J project1 
#SBATCH -n 4
#SBATCH -N 4
#SBATCH -c 24
#SBATCH -t 00:30:00
#SBATCH -o out-4
#SBATCH -p batch
unset I_MPI_PMI_LIBRARY
mpiexec.hydra -bootstrap slurm -l \
  -genv KMP_AFFINITY compact \
 ./stencil-cpp 1000 1000 1000 10 
