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
METRICS = ["pi_3", "pi_2", "pi_1"]

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

fig, ax = plt.subplots(3, 4, layout="constrained")

# one-time cost
for di in range(3): # rows/centrality measures pi3, pi2, pi1
    metric = METRICS[di]
    for case in (11, 51, 101, "ref"): # factor t_=n_*? or reference (independent of t_)
        # one-time cost is d=0 for us and d=1 for reference
        if case == "ref":
            appendix = "d1"
        else:
            appendix = "d0"
        with open(f'{BENCHMARK_DATA_BASE_DIR}/p0/output_{metric}_{case}_{appendix}.txt', 'r') as f0, open(f'{BENCHMARK_DATA_BASE_DIR}/p1/output_{metric}_{case}_{appendix}.txt', 'r') as f1, open(f'{BENCHMARK_DATA_BASE_DIR}/p2/output_{metric}_{case}_{appendix}.txt', 'r') as f2:
            f0l = f0.readlines()
            f1l = f1.readlines()
            f2l = f2.readlines()
        x = []
        y = []
        x_traffic = []
        y_traffic = []
        i = 0

        # Benchmarked run time:
        while i < max(len(f0l),len(f1l),len(f2l)):
            p0 = json.loads(f0l[i])
            p1 = json.loads(f1l[i])
            p2 = json.loads(f2l[i])
            i += 1
            (n, comm_off, comm_on, time_off, time_on, ram) = aggregate_row(p0, p1, p2)
            if n > 50000: # these cases are only rendered in another plot
                continue
            x.append(n)
            y.append((time_off + time_on) / 1000) # ms -> s

        # Compute traffic (preprocessing+online) analytically in bytes.
        # This allows to show communication even where benchmarks did not actually run.
        # See Tab. 4, p. 21 in the paper for number of ring elements.
        # Here, we just multiply by RING_SIZE // 8 to get number of bytes instead.
        # Note that t_ = n_ + #edges.
        # Also note that Tab. 4 reports online per party, so we take 2x that.
        for n_ in range(1, 50001, 100):
            l_ = 3 # assume 3 layers for reference protocol (WWW'17)
            x_traffic.append(n_)
            if case != "ref":
                # total size = n_ + factor(10, 50, or 100) * n_ = case * n_
                t_ = case * n_
            if metric == "pi_3":
                if case == "ref":
                    traffic = 0
                else:
                    traffic = RING_SIZE // 8 * (16*t_*int(math.ceil(math.log2(n_)))+27*t_ + 2*(12*t_*int(math.ceil(math.log2(n_)))+17*t_))
            elif metric == "pi_2":
                if case == "ref":
                    traffic = RING_SIZE // 8 * 5 * (n_**2*(l_-1))
                else:
                    traffic = RING_SIZE // 8 * (24*t_*math.ceil(math.log2(n_))+(55-1/RING_SIZE)*t_-3+1/RING_SIZE + 2*(18*t_*math.ceil(math.log2(n_))+(40-2/RING_SIZE)*t_-6+2/RING_SIZE))
            else:
                assert metric == "pi_1"
                if case == "ref":
                    traffic = RING_SIZE // 8 * 5 * ((2-1/RING_SIZE)*n_**2)
                else:
                    traffic = RING_SIZE // 8 * (16*t_*math.ceil(math.log2(n_))+(4-1/RING_SIZE)*n_*n_ + 2 * n_ * (t_ - n_) + 25*t_ + 2*(12*t_*math.ceil(math.log2(n_))+(5-2/RING_SIZE)*n_*n_ + n_ * (t_ - n_) + 16*t_))
                
            y_traffic.append(traffic)

        # Configure y-axis min and max
        if di == 0:
            # run time:
            if not alt_scale:
                ax[di, 0].set_xlim([0,50000])
                ax[di, 0].set_ylim([0, 250])
            # traffic:
            ax[di, 1].set_xlim([0,50000])
            ax[di, 1].set_ylim([0, 15])
            y_traffic = [d / 1024 / 1024 / 1024 for d in y_traffic]
            unit = "GiB"
        elif di == 1:
            if not alt_scale:
                ax[di, 0].set_xlim([0,40000])
                ax[di, 0].set_ylim([0, 350])
            ax[di, 1].set_xlim([0,40000])
            ax[di, 1].set_ylim([0, 50])
            y_traffic = [d / 1024 / 1024 / 1024 for d in y_traffic]
            unit = "GiB"
        elif di == 2:
            if not alt_scale:
                ax[di, 0].set_xlim([0,900])
                ax[di, 0].set_ylim([0, 40])
            ax[di, 1].set_xlim([0,900])
            ax[di, 1].set_ylim([0, 1.5])
            y_traffic = [d / 1024 / 1024 / 1024 for d in y_traffic]
            unit = "GiB"

        # Configure color, linestyle, x-axis, labels, etc. and plot
        if case == 11:
            col = C1
            lnstyle = "solid"
            label = r"$\bf{ours}$ ($\vert\mathcal{E}\vert=10\vert\mathsf{V}\vert$)"
        elif case == 51:
            col = C2
            lnstyle = "solid"
            label = r"$\bf{ours}$ ($\vert\mathcal{E}\vert=50\vert\mathsf{V}\vert$)"
        elif case == 101:
            col = C3
            lnstyle = "solid"
            label = r"$\bf{ours}$ ($\vert\mathcal{E}\vert=100\vert\mathsf{V}\vert$)"
        else:
            col = C4
            lnstyle = "dotted"
            label = r"prior [6] (arbitrary $\vert\mathcal{E}\vert$)"
        if di == 2:
            ax[di, 0].set_xlabel(r"$\vert\mathsf{v}\vert$",labelpad=0, fontsize=11)
            ax[di, 1].set_xlabel(r"$\vert\mathsf{v}\vert$",labelpad=0, fontsize=11)
        ax[di, 0].set_ylabel("time [s]",labelpad=0, fontsize=9)
        ax[di, 1].set_ylabel(f"comm. [{unit}]",labelpad=0, fontsize=9)
        ax[di, 0].tick_params(axis='both', which='major', labelsize=9)
        ax[di, 1].tick_params(axis='both', which='major', labelsize=9)
        if di == 0:
            if not alt_scale:
                ax[di, 0].set_xticks([0,10000,30000,50000],["0","10k","30k","50k"])
            ax[di, 1].set_xticks([0,10000,30000,50000],["0","10k","30k","50k"])
        elif di == 1:
            if not alt_scale:
                ax[di, 0].set_xticks([0,20000,40000],["0","20k","40k"])
            ax[di, 1].set_xticks([0,20000,40000],["0","20k","40k"])
        else:
            if not alt_scale:
                ax[di, 0].set_xticks([0,400,800])
            ax[di, 1].set_xticks([0,400,800])
        plt.rcParams['axes.titley'] = 1.0
        plt.rcParams['axes.titlepad'] = -12
        if METRICS[di] == "pi_3":
            title = r'$\pi_\mathsf{M}^D$'
        elif METRICS[di] == "pi_2":
            title = r'$\pi_\mathsf{K}^D$'
        elif METRICS[di] == "pi_1":
            title = r'$\pi_\mathsf{R}^D$'
        else:
            assert False
        if di == 0:
            ax[di, 0].plot(x, y, marker=".", color=col, linestyle=lnstyle,label=label)
        else:
            ax[di, 0].plot(x, y, marker=".", color=col, linestyle=lnstyle)
        ax[di, 1].plot(x_traffic, y_traffic, color=col, linestyle=lnstyle)

# cost per iteration
for di in range(3): # rows/centrality measures pi3, pi2, pi1
    metric = METRICS[di]
    for case in (11, 51, 101, "ref"): # factor t_=n_*? or reference (independent of t_)
        # For ref, read data for D=2 and D=1.
        # For ours, read data for D=2, D=1, and D=0.
        # Stragegy after that:
        # Cost for one reference iteration is cost for D=2 minus cost for D=1.
        # Cost for one of our iterations is cost for D=2 minus cost for D=0, then divided by 2.
        # This is relevant as our iterations are relatively cheap so that averaging over 2 iterations
        # compensates some statistical fluctuation.
        appendix = "d2"
        prior_appendix = "d1"
        pp_appendix = "d0"
        if case == "ref":
            with open(f'{BENCHMARK_DATA_BASE_DIR}/p0/output_{metric}_{case}_{appendix}.txt', 'r') as f0, open(f'{BENCHMARK_DATA_BASE_DIR}/p1/output_{metric}_{case}_{appendix}.txt', 'r') as f1, open(f'{BENCHMARK_DATA_BASE_DIR}/p2/output_{metric}_{case}_{appendix}.txt',    'r') as f2, open(f'{BENCHMARK_DATA_BASE_DIR}/p0/output_{metric}_{case}_{prior_appendix}.txt', 'r') as f0p, open(f'{BENCHMARK_DATA_BASE_DIR}/p1/output_{metric}_{case}_{prior_appendix}.txt', 'r') as f1p, open(f'{BENCHMARK_DATA_BASE_DIR}/p2/output_{metric}_{case}_{prior_appendix}.txt', 'r') as f2p:
                f0l = f0.readlines()
                f1l = f1.readlines()
                f2l = f2.readlines()
                f0lp = f0p.readlines()
                f1lp = f1p.readlines()
                f2lp = f2p.readlines()
        else:
            with open(f'{BENCHMARK_DATA_BASE_DIR}/p0/output_{metric}_{case}_{appendix}.txt', 'r') as f0, open(f'{BENCHMARK_DATA_BASE_DIR}/p1/output_{metric}_{case}_{appendix}.txt', 'r') as f1, open(f'{BENCHMARK_DATA_BASE_DIR}/p2/output_{metric}_{case}_{appendix}.txt',    'r') as f2, open(f'{BENCHMARK_DATA_BASE_DIR}/p0/output_{metric}_{case}_{prior_appendix}.txt', 'r') as f0p, open(f'{BENCHMARK_DATA_BASE_DIR}/p1/output_{metric}_{case}_{prior_appendix}.txt', 'r') as f1p, open(f'{BENCHMARK_DATA_BASE_DIR}/p2/output_{metric}_{case}_{prior_appendix}.txt', 'r') as f2p, open(f'{BENCHMARK_DATA_BASE_DIR}/p0/output_{metric}_{case}_{pp_appendix}.txt', 'r') as f0p2, open(f'{BENCHMARK_DATA_BASE_DIR}/p1/output_{metric}_{case}_{pp_appendix}.txt', 'r') as f1p2, open(f'{BENCHMARK_DATA_BASE_DIR}/p2/output_{metric}_{case}_{pp_appendix}.txt', 'r') as f2p2:
                f0l = f0.readlines()
                f1l = f1.readlines()
                f2l = f2.readlines()
                f0lp = f0p.readlines()
                f1lp = f1p.readlines()
                f2lp = f2p.readlines()
                f0lp2 = f0p2.readlines()
                f1lp2 = f1p2.readlines()
                f2lp2 = f2p2.readlines()

        x = []
        y = []
        x_traffic = []
        y_traffic = []
        i = 0

        # Benchmarked run time:
        while i < max(len(f0l),len(f1l),len(f2l),len(f0lp),len(f1lp),len(f2lp)):
            p0p2 = None
            p1p2 = None
            p2p2 = None
            p0 = json.loads(f0l[i])
            p1 = json.loads(f1l[i])
            p2 = json.loads(f2l[i])
            p0p = json.loads(f0lp[i])
            p1p = json.loads(f1lp[i])
            p2p = json.loads(f2lp[i])
            if case != "ref": # data for D=0 not loaded for reference
                p0p2 = json.loads(f0lp2[i])
                p1p2 = json.loads(f1lp2[i])
                p2p2 = json.loads(f2lp2[i])
            i += 1
            (n, comm_off, comm_on, time_off, time_on, ram) = aggregate_row(p0, p1, p2)
            (np, comm_offp, comm_onp, time_offp, time_onp, ramp) = aggregate_row(p0p, p1p, p2p)
            if case != "ref":
                (np2, comm_offp2, comm_onp2, time_offp2, time_onp2, ramp2) = aggregate_row(p0p2, p1p2, p2p2)
            assert n == np
            if n > 50000: # these cases are only rendered in another plot
                continue
            x.append(n)
            if case == "ref":
                y.append((time_off + time_on - time_offp - time_onp) / 1000) # ms -> s
            else:
                # average over 2 iterations, see longer comment above
                y.append((time_off + time_on - time_offp2 - time_onp2) / 1000 / 2) # ms -> s

        # Compute traffic (preprocessing+online) analytically in bytes.
        # This allows to show communication even where benchmarks did not actually run.
        # See Tab. 4, p. 21 in the paper for number of ring elements.
        # Here, we just multiply by RING_SIZE // 8 to get number of bytes instead.
        # Note that t_ = n_ + #edges.
        # Also note that Tab. 4 reports online per party, so we take 2x that.
        for n_ in range(1, 50001, 100):
            l_ = 3 # assume 3 layers for reference protocol (WWW'17)
            x_traffic.append(n_)
            if case != "ref":
                # total size = n_ + factor(10, 50, or 100) * n_ = case * n_
                t_ = case * n_
            if metric == "pi_3":
                if case == "ref":
                    traffic = RING_SIZE // 8 * 5 * (n_**2)
                else:
                    traffic = RING_SIZE // 8 * (8*t_ + 2*(4*t_))
            elif metric == "pi_2":
                if case == "ref":
                    traffic = RING_SIZE // 8 * 5 * (n_**2)
                else:
                    traffic = RING_SIZE // 8 * (8*t_ + 2*(4*t_))
            else:
                assert metric == "pi_1"
                if case == "ref":
                    traffic = RING_SIZE // 8 * 5 * (n_**3)
                else:
                    traffic = RING_SIZE // 8 * (8*t_*n_ + 2*(4*t_*n_))
                
            y_traffic.append(traffic)

        # Configure y-axis min and max
        if di == 0:
            # run time:
            if not alt_scale:
                ax[di, 2].set_xlim([0,50000])
                ax[di, 2].set_ylim([0, 20])
            # traffic:
            ax[di, 3].set_xlim([0,50000])
            ax[di, 3].set_ylim([0, 1])
            y_traffic = [d / 1024 / 1024 / 1024 for d in y_traffic]
            unit = "GiB"
        elif di == 1:
            if not alt_scale:
                ax[di, 2].set_xlim([0,40000])
                ax[di, 2].set_ylim([0, 20])
            ax[di, 3].set_xlim([0,40000])
            ax[di, 3].set_ylim([0, 1])
            y_traffic = [d / 1024 / 1024 / 1024 for d in y_traffic]
            unit = "GiB"
        elif di == 2:
            if not alt_scale:
                ax[di, 2].set_xlim([0,900])
                ax[di, 2].set_ylim([0, 80])
            ax[di, 3].set_xlim([0,900])
            ax[di, 3].set_ylim([0, 15])
            y_traffic = [d / 1024 / 1024 / 1024 for d in y_traffic]
            unit = "GiB"

        # Configure color, linestyle, x-axis, labels, etc. and plot
        if case == 11:
            col = C1
            lnstyle = "solid"
            label = r"$\bf{ours}$ ($\vert\mathcal{E}\vert=10\vert\mathsf{V}\vert$)"
        elif case == 51:
            col = C2
            lnstyle = "solid"
            label = r"$\bf{ours}$ ($\vert\mathcal{E}\vert=50\vert\mathsf{V}\vert$)"
        elif case == 101:
            col = C3
            lnstyle = "solid"
            label = r"$\bf{ours}$ ($\vert\mathcal{E}\vert=100\vert\mathsf{V}\vert$)"
        else:
            col = C4
            lnstyle = "dotted"
            label = r"prior [6] (arbitrary $\vert\mathcal{E}\vert$)"
        if di == 2:
            ax[di, 2].set_xlabel(r"$\vert\mathsf{v}\vert$",labelpad=0, fontsize=11)
            ax[di, 3].set_xlabel(r"$\vert\mathsf{v}\vert$",labelpad=0, fontsize=11)
        ax[di, 2].set_ylabel("time [s]",labelpad=0, fontsize=9)
        ax[di, 3].set_ylabel(f"comm. [{unit}]",labelpad=0, fontsize=9)
        ax[di, 2].tick_params(axis='both', which='major', labelsize=9)
        ax[di, 3].tick_params(axis='both', which='major', labelsize=9)
        if di == 0:
            if not alt_scale:
                ax[di, 2].set_xticks([0,10000,30000,50000],["0","10k","30k","50k"])
            ax[di, 3].set_xticks([0,10000,30000,50000],["0","10k","30k","50k"])
        elif di == 1:
            if not alt_scale:
                ax[di, 2].set_xticks([0,20000,40000],["0","20k","40k"])
            ax[di, 3].set_xticks([0,20000,40000],["0","20k","40k"])
        else:
            if not alt_scale:
                ax[di, 2].set_xticks([0,400,800])
            ax[di, 3].set_xticks([0,400,800])
        plt.rcParams['axes.titley'] = 1.0
        plt.rcParams['axes.titlepad'] = -12
        if METRICS[di] == "pi_3":
            title = r'$\pi_\mathsf{M}^D$'
        elif METRICS[di] == "pi_2":
            title = r'$\pi_\mathsf{K}^D$'
        elif METRICS[di] == "pi_1":
            title = r'$\pi_\mathsf{R}^D$'
        else:
            assert False
        ax[di, 2].plot(x, y, marker=".", color=col, linestyle=lnstyle)
        ax[di, 3].plot(x_traffic, y_traffic, color=col, linestyle=lnstyle)

# final formatting
fig.align_ylabels(ax[:, 0])
fig.align_ylabels(ax[:, 1])
fig.align_ylabels(ax[:, 2])
fig.align_ylabels(ax[:, 3])
fig.set_size_inches(9, 3.8)

fig.get_layout_engine().set(w_pad = 10 / 72, h_pad = 2 / 72, hspace=0,
                            wspace=0)
fig.get_layout_engine().set(rect=(0.06,0.07,0.94,0.85))
lines_labels = [ax.get_legend_handles_labels() for ax in fig.axes]
lines, labels = [sum(lol, []) for lol in zip(*lines_labels)]
fig.legend(lines, labels, loc=(0.07,0), ncol=4, prop={'size': 10},framealpha=1)
fig.text(0.23, 0.95, "one-time cost", fontsize=12)
fig.text(0.68, 0.95, "cost per iteration", fontsize=12)

line = plt.Line2D([0.532,0.532],[0,1], transform=fig.transFigure, color="black")
fig.add_artist(line)

fig.text(0.01, 0.93 - 0.5 * (0.83) / 3, r"$\pi_\mathsf{M}^D$", fontsize=12)
fig.text(0.01, 0.93 - 1.5 * (0.83) / 3, r"$\pi_\mathsf{K}^D$", fontsize=12)
fig.text(0.01, 0.93 - 2.5 * (0.83) / 3, r"$\pi_\mathsf{R}^D$", fontsize=12)

rc('pdf', fonttype=42)
plt.savefig('scalability_plots_betterspacing.pdf')
plt.show()
