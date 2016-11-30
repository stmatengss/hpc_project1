#!/bin/bash
#SBATCH -J project1 
#SBATCH -n 1 
#SBATCH -N 1 
#SBATCH -c 24
#SBATCH -t 00:30:00
#SBATCH -o out-1
#SBATCH -p batch
unset I_MPI_PMI_LIBRARY
mpiexec.hydra -bootstrap slurm -l \
  -genv KMP_AFFINITY compact \
 ./stencil-cpp 1000 1000 1000 10 
