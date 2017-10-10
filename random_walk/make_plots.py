#!/usr/bin/env python3

from subprocess import run

import matplotlib.pyplot as plt
import numpy as np

def simulation_time_omp(a=0, b=10, c=5, runs=500, p=0.5, thread_num=4):
    run([str(arg) for arg in ['./random_walk_omp', a, b, c, runs, p, thread_num]])

    with open('stats.txt', 'r') as stats:
        return float(stats.read().split()[2])

def simulation_time_base(a=0, b=10, c=5, runs=500, p=0.5):
    run([str(arg) for arg in ['./random_walk_simple', a, b, c, runs, p]])

    with open('stats.txt', 'r') as stats:
        return float(stats.read().split()[2])

def plot_tse_vs_n(ns, thread_nums):
    fig, axes = plt.subplots(len(thread_nums), 3, figsize=(21, len(thread_nums) * 5))

    base_ts = np.array([simulation_time_base(runs=n) for n in ns])

    for i, thread_num in enumerate(thread_nums):
        t_vs_n_ax = axes[i, 0]
        s_vs_n_ax = axes[i, 1]
        e_vs_n_ax = axes[i, 2]

        s_vs_n_ax.set_title("P = {}".format(thread_num))

        t_vs_n_ax.set_xlabel("N")
        s_vs_n_ax.set_xlabel("N")
        e_vs_n_ax.set_xlabel("N")
        t_vs_n_ax.set_ylabel("T")
        s_vs_n_ax.set_ylabel("S")
        e_vs_n_ax.set_ylabel("E")

        ts = np.array([simulation_time_omp(runs=n, thread_num=thread_num)
                           for n in ns])
        
        t_vs_n_ax.plot(ns, ts)
        s_vs_n_ax.plot(ns, base_ts / ts)
        e_vs_n_ax.plot(ns, base_ts / ts / thread_num)

    return fig

def plot_tse_vs_p(n, thread_nums):
    fig, axes = plt.subplots(1, 3, figsize=(21, 5))

    base_ts = simulation_time_base(runs=n)

    t_vs_n_ax = axes[0]
    s_vs_n_ax = axes[1]
    e_vs_n_ax = axes[2]

    s_vs_n_ax.set_title("N = {}".format(n))

    t_vs_n_ax.set_xlabel("P")
    s_vs_n_ax.set_xlabel("P")
    e_vs_n_ax.set_xlabel("P")
    t_vs_n_ax.set_ylabel("T")
    s_vs_n_ax.set_ylabel("S")
    e_vs_n_ax.set_ylabel("E")

    ts = np.array([simulation_time_omp(runs=n, thread_num=thread_num)
                       for thread_num in thread_nums])
    
    t_vs_n_ax.plot(thread_nums, ts)
    s_vs_n_ax.plot(thread_nums, base_ts / ts)
    e_vs_n_ax.plot(thread_nums, base_ts / ts / thread_nums)

    return fig

ns = np.arange(100000, 500001, 50000)
thread_nums = np.array([1, 2, 4, 8, 16])

plot_tse_vs_n(ns, thread_nums).savefig("tse_vs_n.png")
plot_tse_vs_p(ns[-1], thread_nums).savefig("tse_vs_p.png")
