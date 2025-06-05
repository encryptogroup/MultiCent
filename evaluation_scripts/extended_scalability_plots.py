import sys
import json
import statistics
import matplotlib.pyplot as plt
import math
from matplotlib import rc

C1 = (0.12156862745098039, 0.4666666666666667, 0.7058823529411765)
C2 = (1.0, 0.4980392156862745, 0.054901960784313725)
C3 = (0.17254901960784313, 0.6274509803921569, 0.17254901960784313)
C4 = (0.8392156862745098, 0.15294117647058825, 0.1568627450980392)

RING_SIZE = 32
if len(sys.argv) > 1 and sys.argv[1] == "original":
    BENCHMARK_DATA_BASE_DIR = "raw_benchmark_outputs"
else:
    BENCHMARK_DATA_BASE_DIR = "../build/benchmarks"
metric = "pi_3"

alt_scale = len(sys.argv) > 1 and sys.argv[1] == "altScale"

def aggregate_row(p0, p1, p2):
    # Aggregates benchmark row for parties p0, p1, p2.
    # Regarding communication, we are interested in the global value, so we compute the sum over all.
    # Regarding run time, we are interested in the maximum among parties.
    # Each operation done for preprocessing/setup (_pre) and online (no suffix).
    comm_off = []
    comm_on = []
    time_off = []
    time_on = []
    rep = len(p0['benchmarks_pre'])
    for r in range(rep):
        comm_off.append(sum(p0['benchmarks_pre'][r]['communication']))
        comm_on.append(sum(p1['benchmarks'][r]['communication']))
        assert sum(p0['benchmarks'][r]['communication']) == 0 # helper does not communicate online
        assert sum(p1['benchmarks_pre'][r]['communication']) == 0 # P1 does not send offline
        assert sum(p2['benchmarks_pre'][r]['communication']) == 0 # P2 does not send offline
        assert sum(p1['benchmarks'][r]['communication']) == sum(p2['benchmarks'][r]['communication']) # P1/P2 send equally online
        time_off.append(max(p0['benchmarks_pre'][r]['time'],p1['benchmarks_pre'][r]['time'],p2['benchmarks_pre'][r]['time']))
        time_on.append(max(p0['benchmarks'][r]['time'],p1['benchmarks'][r]['time'],p2['benchmarks'][r]['time']))
    assert comm_off[0] == statistics.mean(comm_off)
    assert comm_on[0] == statistics.mean(comm_on)
    comm_off = comm_off[0]
    comm_on = comm_on[0]
    time_off = statistics.mean(time_off)
    time_on = statistics.mean(time_on)
    ram = max(p0['stats']['peak_virtual_memory'], p1['stats']['peak_virtual_memory'], p2['stats']['peak_virtual_memory'])
    n = p0['details']['nodes']
    assert n == p1['details']['nodes']
    assert n == p2['details']['nodes']
    return (n, comm_off, comm_on, time_off, time_on, ram)

fig, ax = plt.subplots(2, 2, layout="constrained")

case = "11" # factor t_=n_*? or reference (independent of t_)

# Read data for D=2, D=1, and D=0.
# Stragegy after that:
# Cost for one reference iteration is cost for D=2 minus cost for D=1.
# Cost for one of our iterations is cost for D=2 minus cost for D=0, then divided by 2.
# This is relevant as our iterations are relatively cheap so that averaging over 2 iterations
# compensates some statistical fluctuation.
with open(f'{BENCHMARK_DATA_BASE_DIR}/p0/output_{metric}_{case}_d0.txt', 'r') as f0, open(f'{BENCHMARK_DATA_BASE_DIR}/p1/output_{metric}_{case}_d0.txt', 'r') as f1, open(f'{BENCHMARK_DATA_BASE_DIR}/p2/output_{metric}_{case}_d0.txt', 'r') as f2, open(f'{BENCHMARK_DATA_BASE_DIR}/p0/output_{metric}_{case}_d1.txt', 'r') as f0d1, open(f'{BENCHMARK_DATA_BASE_DIR}/p1/output_{metric}_{case}_d1.txt', 'r') as f1d1, open(f'{BENCHMARK_DATA_BASE_DIR}/p2/output_{metric}_{case}_d1.txt', 'r') as f2d1, open(f'{BENCHMARK_DATA_BASE_DIR}/p0/output_{metric}_{case}_d2.txt', 'r') as f0d2, open(f'{BENCHMARK_DATA_BASE_DIR}/p1/output_{metric}_{case}_d2.txt', 'r') as f1d2, open(f'{BENCHMARK_DATA_BASE_DIR}/p2/output_{metric}_{case}_d2.txt', 'r') as f2d2:
    f0l = f0.readlines()
    f1l = f1.readlines()
    f2l = f2.readlines()
    f0ld1 = f0d1.readlines()
    f1ld1 = f1d1.readlines()
    f2ld1 = f2d1.readlines()
    f0ld2 = f0d2.readlines()
    f1ld2 = f1d2.readlines()
    f2ld2 = f2d2.readlines()

x = []
x_traffic = []
# communication: online (on) and total
y_once_total = []
y_once_on = []
y_perIter_total = []
y_perIter_on = []
y_once_total_traffic = []
y_once_on_traffic = []
y_perIter_total_traffic = []
y_perIter_on_traffic = []
i = 0

# Benchmarked run time:
while i < max(len(f0l),len(f1l),len(f2l),len(f0ld1),len(f1ld1),len(f2ld1),len(f0ld2),len(f1ld2),len(f2ld2)):
    p0 = json.loads(f0l[i])
    p1 = json.loads(f1l[i])
    p2 = json.loads(f2l[i])
    p0d1 = json.loads(f0ld1[i])
    p1d1 = json.loads(f1ld1[i])
    p2d1 = json.loads(f2ld1[i])
    p0d2 = json.loads(f0ld2[i])
    p1d2 = json.loads(f1ld2[i])
    p2d2 = json.loads(f2ld2[i])
    i += 1
    (n, comm_off, comm_on, time_off, time_on, ram) = aggregate_row(p0, p1, p2)
    (nd1, comm_offd1, comm_ond1, time_offd1, time_ond1, ramd1) = aggregate_row(p0d1, p1d1, p2d1)
    (nd2, comm_offd2, comm_ond2, time_offd2, time_ond2, ramd2) = aggregate_row(p0d2, p1d2, p2d2)
    assert n == nd1
    assert n == nd2
    x.append(n)
    y_once_total.append((time_off + time_on) / 1000) # ms -> s
    y_once_on.append(time_on / 1000) # ms -> s
    # average over 2 iterations, see longer comment above
    y_perIter_total.append((time_offd2 + time_ond2 - time_off - time_on) / 1000 / 2) # ms -> s
    y_perIter_on.append((time_ond2 - time_on) / 1000 / 2) # ms -> s

# Compute traffic (preprocessing+online) analytically in bytes.
# This allows to show communication even where benchmarks did not actually run.
# See Tab. 4, p. 21 in the paper for number of ring elements.
# Here, we just multiply by RING_SIZE // 8 to get number of bytes instead.
# Note that t_ = n_ + #edges.
# Also note that Tab. 4 reports online per party, so we take 2x that.
for n_ in range(1, 500001, 100):
    x_traffic.append(n_)
    # total size = n_ + factor(10, 50, or 100) * n_ = case * n_
    t_ = 11 * n_
    y_once_total_traffic.append(RING_SIZE // 8 * (16*t_*int(math.ceil(math.log2(n_)))+27*t_ + 2*(12*t_*int(math.ceil(math.log2(n_)))+17*t_)) / 1024 / 1024 / 1024)
    y_once_on_traffic.append(RING_SIZE // 8 * (2*(12*t_*int(math.ceil(math.log2(n_)))+17*t_)) / 1024 / 1024 / 1024)
    y_perIter_total_traffic.append(RING_SIZE // 8 * (8*t_ + 2*(4*t_)) / 1024 / 1024)
    y_perIter_on_traffic.append(RING_SIZE // 8 * (2*(4*t_)) / 1024 / 1024)

# one-time run time:
if not alt_scale:
    ax[0, 0].set_xlim([0, 500000])
    ax[0, 0].set_ylim([0, 350])
    ax[0, 0].set_xticks([0,100000,200000,300000,400000,500000],["0","100k","200k","200k","400k","500k"])
ax[0, 0].set_ylabel("time [s]",labelpad=0, fontsize=9)
ax[0, 0].tick_params(axis='both', which='major', labelsize=9)
ax[0, 0].plot(x, y_once_on, marker=".", label="online")
ax[0, 0].plot(x, y_once_total, marker=".", label="total (preprocessing + online)")
plt.rcParams['axes.titley'] = 1.0
plt.rcParams['axes.titlepad'] = -12
# one-time traffic:
ax[1, 0].set_xlim([0, 500000])
ax[1, 0].set_ylim([0, 18])
ax[1, 0].set_xticks([0,100000,200000,300000,400000,500000],["0","100k","200k","200k","400k","500k"])
ax[1, 0].set_ylabel("comm. [GiB]",labelpad=0, fontsize=9)
ax[1, 0].tick_params(axis='both', which='major', labelsize=9)
ax[1, 0].set_xlabel(r"$\vert\mathsf{v}\vert$",labelpad=0, fontsize=11)
ax[1, 0].plot(x_traffic, y_once_on_traffic)
ax[1, 0].plot(x_traffic, y_once_total_traffic)
# per iteration run time
if not alt_scale:
    ax[0, 1].set_xlim([0, 500000])
    ax[0, 1].set_ylim([0, 9])
    ax[0, 1].set_xticks([0,100000,200000,300000,400000,500000],["0","100k","200k","200k","400k","500k"])
ax[0, 1].set_ylabel("time [s]",labelpad=0, fontsize=9)
ax[0, 1].tick_params(axis='both', which='major', labelsize=9)
ax[0, 1].plot(x, y_perIter_on, marker=".")
ax[0, 1].plot(x, y_perIter_total, marker=".")
# per iteration traffic
ax[1, 1].set_xlim([0, 500000])
ax[1, 1].set_ylim([0, 350])
ax[1, 1].set_xticks([0,100000,200000,300000,400000,500000],["0","100k","200k","200k","400k","500k"])
ax[1, 1].set_ylabel("comm [MiB]",labelpad=0, fontsize=9)
ax[1, 1].set_xlabel(r"$\vert\mathsf{v}\vert$",labelpad=0, fontsize=11)
ax[1, 1].tick_params(axis='both', which='major', labelsize=9)
ax[1, 1].plot(x_traffic, y_perIter_on_traffic)
ax[1, 1].plot(x_traffic, y_perIter_total_traffic)

# final formatting
fig.align_ylabels(ax[:, 0])
fig.align_ylabels(ax[:, 1])
fig.set_size_inches(6.8, 4)

fig.get_layout_engine().set(w_pad = 12 / 72, h_pad = 2 / 72, hspace=0,
                            wspace=0)
fig.get_layout_engine().set(rect=(0.0,0.06,1,0.87))
lines_labels = [ax.get_legend_handles_labels() for ax in fig.axes]
lines, labels = [sum(lol, []) for lol in zip(*lines_labels)]
fig.legend(lines, labels, loc=(0.2,0), ncol=4, prop={'size': 11},framealpha=1)

fig.text(0.17, 0.95, "one-time cost", fontsize=12)
fig.text(0.65, 0.95, "cost per iteration", fontsize=12)

line = plt.Line2D([0.498,0.498],[0,1], transform=fig.transFigure, color="black")
fig.add_artist(line)
rc('pdf', fonttype=42)
plt.savefig('extended_scalability_plots.pdf')
plt.show()
