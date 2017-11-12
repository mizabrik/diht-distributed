#!/usr/bin/env python3

from subprocess import run, PIPE

import matplotlib.pyplot as plt
import numpy as np

def solve_tsp(thread_num=4, N=100000, S=7, graph='graph.txt'):
    p = run([str(arg) for arg in ['./tsp', thread_num, N, S, '--file', graph, '--verbose']],
            stdout=PIPE, encoding='utf8')

    return np.array([float(x) for x in p.stdout.split()]).reshape((-1, 3)).T

def plot_progress(populations):
    height = (len(populations) + 1) // 2
    fig, axes = plt.subplots(height, 2, figsize=(21, 8 * height))

    for i, population in enumerate(populations):
        print("Running with population size {}".format(population))
        mins, maxs, means = solve_tsp(N=population)
        t = np.arange(means.size)
        axes[i // 2, i % 2].set_title("N = {}".format(population))
        axes[i // 2, i % 2].plot(t, mins)
        axes[i // 2, i % 2].plot(t, maxs)
        axes[i // 2, i % 2].plot(t, means)

    return fig

if __name__ == '__main__':
    populations = np.arange(10000, 100001, 10000)
    
    with open('graph.txt', 'w') as g:
        run(['./generate', '1000'], stdout=g)
    plot_progress(populations).savefig("progress.png")
