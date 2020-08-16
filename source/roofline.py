import matplotlib
from pprint import pprint
import numpy as np
import matplotlib.pyplot as plt

def frange(start, stop, step=1.0):
    f = start
    while f < stop:
        f += step
        yield f


plt.rcParams["font.family"] = "Times New Roman"
#color_list = ['C8', 'C2', 'C9', 'C1']
color_list = ['C1', 'C2', 'C3', 'C4', 'C5', 'C6', 'C7', 'C8', 'C9', 'C10']
marker_list = ['o', 's', 'p', '*', 'h', 'H', 'd', '+', 'v', 'x']

# Input (usually from ECM model)
result = {
    'min performance': 11175000000.0, 'bottleneck level': 2,
    'mem bottlenecks': [{'performance': [24474545454.545452, '', 'FLOP/s'],
                         'bandwidth': [89.74, u'G', u'B/s'],
                         'arithmetic intensity': 0.2727272727272727,
                         'bw kernel': 'triad', 'level': 'L1-L2'},
                        {'performance': [12957000000.0, '',
                                         'FLOP/s'], 'bandwidth': [43.19, u'G', u'B/s'],
                         'arithmetic intensity': 0.3, 'bw kernel': 'triad', 'level': 'L2-L3'},
                        {'performance': [11175000000.0, '', 'FLOP/s'],
                         'bandwidth': [22.35, u'G', u'B/s'],
                         'arithmetic intensity': 0.5, 'bw kernel': 'triad', 'level': 'L3-MEM'}]}
#machine = yaml.load(open('machine-files/emmy.yaml'))
#max_flops = 614.4 #machine['clock']*sum(machine['FLOPs per cycle']['DP'].values())

Freq = [ 2.4, 2.0, 1.3 ] # GHz
Core = [ 32, 32, 64 ]  # 32 cores
ThreadsPerCore = [ 4, 4, 4 ] # Estimated
SIMD_width = [ 2, 8, 8 ]

PEAK= [ 2 * Freq[i] * Core[i] * SIMD_width[i] for i in range(0,3) ] # 2 SIMD parts in a core

max_flops = PEAK
max_flops = [811.0]
labels = ['SW26010-1CG']

pprint(result)
pprint(max_flops)

# Plot configuration
height = 0.8
fig = plt.figure(frameon=False)
ax = fig.add_subplot(1, 1, 1)

yticks_labels = []
yticks = []
xticks_labels = []
xticks = [2.**i for i in range(-4, 10)]

ax.set_xlabel('Operational Intensity (Flops/Byte, log scale) ', fontsize=12)
ax.set_ylabel('Performance (GFlops, log scale) ', fontsize=12)
ylim_min, ylim_max = ax.get_ylim()
xlim_min, xlim_max = ax.get_xlim()

#ai_list = [1.4874, 3.25490, 0.3258, 0.3687]
#gflops_list = [9.8807, 41.5006, 6.2, 7.0275]
#labels_list = ['naive LLD', 'LLD', 'naive LLL', 'LLL']

ai_list = [0.25, 0.47, 0.5, 0.9, 5.1, 5.1, 0.22, 0.22, 0.125, 0.125]
gflops_list = [0.092, 0.033, 0.001, 0.027, 10.234, 27.445, 0.001, 0.025, 0.002, 1.096]
labels_list = ['backprop openACC', 'backprop athread', 'hotspot3D openACC', 'hotspot3D athread', 'kmeans openACC', 'kmeans athread', 'nw openACC', 'nw athread', 'pathfinder openACC', 'pathfinder athread']

for i in range(len(ai_list)):
    #plt.plot(ai_list[i], gflops_list[i], color=color_list[i], marker='^', label=labels_list[i])
    plt.plot(ai_list[i], gflops_list[i], color=color_list[i], marker=marker_list[i], label=labels_list[i])
    ax.vlines(ai_list[i], ylim_min, gflops_list[i], colors = "grey", linewidth=1, linestyles = "--")
plt.legend(loc='best', fontsize=12)

membw = [22.5]

# Upper bound
x = list(frange(min(xticks), max(xticks), 0.01))
bw = membw[0]
ax.plot(x, [min(bw*x, float(max_flops[0])) for x in x], label=labels[0], color='black', linewidth=1)

ax.plot(max_flops[0]/bw, max_flops[0], color='black')
ax.vlines(max_flops[0]/bw, ylim_min, max_flops[0], colors = "grey", linewidth=1, linestyles = "--")
plt.text(max_flops[0]/bw, 0.001, "%.01lf" % (max_flops[0]/bw), horizontalalignment='center')

# memory bound & compute bound
annotate_height = 100
# 箭头
ax.annotate("", xy=(30, annotate_height), xytext=(5, annotate_height),arrowprops=dict(arrowstyle="<-", color='tab:orange'))
ax.annotate("", xy=(50, annotate_height), xytext=(300, annotate_height),arrowprops=dict(arrowstyle="<-", color='tab:cyan'))
# 箭头说明
plt.text(12, annotate_height-40, "Memory Bound", horizontalalignment='center')
plt.text(140, annotate_height-40, "Computing Bound", horizontalalignment='center')


ax.text(1.5, 15, "peak stream bandwidth 22.5GB/s", horizontalalignment='center', rotation=15, fontsize=12)
ax.text(70, 200, "peak performance 811.0 GFlops", horizontalalignment='center', rotation=0, fontsize=12)
ax.set_xscale('log')
ax.set_yscale('log')
ax.set_xlim(min(xticks)+0.01, max(xticks))

# ax.set_yticks([perf, float(max_flops)])
#ax.set_xticks(xticks+arith_intensity)
#ax.grid(axis='x', alpha=0.7, linestyle='--')
fig.savefig('./roofline.pdf', bbox_inches = 'tight')
fig.savefig('./roofline.png', bbox_inches = 'tight')
plt.show()
