#!/usr/bin/env python3

from subprocess import run

import matplotlib.pyplot as plt
import numpy as np

def sort_time(n=10000000, m=100000, thread_num=0):
    run([str(arg) for arg in ['./sort', n, m, thread_num]])

    with open('stats.txt', 'r') as stats:
        return float(stats.read().split()[0])

def plot_tse_vs_p(n, m, thread_nums):
    fig, axes = plt.subplots(1, 3, figsize=(21, 5))

    t_vs_n_ax = axes[0]
    s_vs_n_ax = axes[1]
    e_vs_n_ax = axes[2]

    s_vs_n_ax.set_title("n = {}, m = {}".format(n, m))

    t_vs_n_ax.set_xlabel("P")
    s_vs_n_ax.set_xlabel("P")
    e_vs_n_ax.set_xlabel("P")
    t_vs_n_ax.set_ylabel("T")
    s_vs_n_ax.set_ylabel("S")
    e_vs_n_ax.set_ylabel("E")

    ts = np.array([sort_time(n, m, thread_num)
                       for thread_num in thread_nums])
    
    t_vs_n_ax.plot(thread_nums, ts)
    s_vs_n_ax.plot(thread_nums, ts[0] / ts)
    e_vs_n_ax.plot(thread_nums, ts[0] / ts / thread_nums)

    t_vs_n_ax.axhline(sort_time(n, m, 0))

    return fig

n = 10000000
m = 800000
thread_nums = np.array([1, 2, 4, 8, 16])

plot_tse_vs_p(n, m, thread_nums).savefig("tse_vs_p.png")
