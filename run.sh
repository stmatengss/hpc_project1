#!/bin/sh
make
cat out >> log
cat out-bench >> log-bench
sbatch submit.sh
sbatch submit-bench.sh
sleep 10
cat out
