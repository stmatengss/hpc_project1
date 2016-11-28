#!/bin/sh
make
cat out >> log
sbatch submit.sh
