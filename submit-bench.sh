#!/bin/bash
#SBATCH -J project1 
#SBATCH -n 1 
#SBATCH -N 1 
#SBATCH -c 24
#SBATCH -t 00:30:00
#SBATCH -o out-bench 
#SBATCH -p batch
unset I_MPI_PMI_LIBRARY
mpiexec.hydra -bootstrap slurm -l \
  -genv KMP_AFFINITY compact \
 ./stencil-bench 16 16 16 10 
