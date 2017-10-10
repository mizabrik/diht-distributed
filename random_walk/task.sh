#!/bin/sh

#SBATCH --job-name="Random walk"
#SBATCH --time=00:10:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=16

srun -n 16 python make_plots.py
