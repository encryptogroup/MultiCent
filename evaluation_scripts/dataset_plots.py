import sys
import json
import statistics
import matplotlib.pyplot as plt
import math
from matplotlib import rc

C1 = (0.12156862745098039, 0.4666666666666667, 0.7058823529411765)
C2 = (1.0, 0.4980392156862745, 0.054901960784313725)
C3 = (0.17254901960784313, 0.6274509803921569, 0.17254901960784313)

DATASETS = ["aarhus", "london", "hiv", "arabi", "higgs"]
RING_SIZE = 32
if len(sys.argv) > 1 and sys.argv[1] == "original":
    BENCHMARK_DATA_BASE_DIR = "raw_benchmark_outputs"
else:
    BENCHMARK_DATA_BASE_DIR = "../build/benchmarks"

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
    return (comm_off, comm_on, time_off, time_on, ram)

fig, ax = plt.subplots(5, 2, layout="constrained")

for di in range(5): # rows/datasets: aarhus, london, hiv, arabi, higgs
    dataset = DATASETS[di]
    # n_: number of nodes
    # t_: total size = n_ + number of edges (in undirected graphs, each edge counts twice as we need
    #   to consider both directions). Not relevant for reference (WWW'17)
    # l_: Number of layers. Only relevant for reference (WWW'17)
    if dataset == "aarhus":
        n_ = 61
        t_ = 1301
        l_ = 5
    elif dataset == "london":
        n_ = 369
        t_ = 1375
        l_ = 3
    elif dataset == "hiv":
        n_ = 1005
        t_ = 3693
        l_ = 3
    elif dataset == "arabi":
        n_ = 6980
        t_ = 43214
        l_ = 7
    elif dataset == "higgs":
        n_ = 304691
        t_ = 1415653
        l_ = 3
    benchmarked = True
    for subp in range(6): # per dataset: pi3, pi2, pi1, reference pi3, r. pi2, r. pi1
        if subp == 0:
            metric = "pi_3"
        elif subp == 1:
            metric = "pi_2"
        elif subp == 2:
            if di > 2:
                benchmarked = False
            metric = "pi_1"
        elif subp == 3:
            if di > 2:
                benchmarked = False
            metric = "pi_3_ref"
        elif subp == 4:
            if di > 2:
                benchmarked = False
            metric = "pi_2_ref"
        elif subp == 5:
            if di > 2:
                benchmarked = False
            metric = "pi_1_ref"
        else:
            assert False

        if benchmarked:
            with open(f'{BENCHMARK_DATA_BASE_DIR}/p0/output_{metric}_{dataset}.txt', 'r') as f0, open(f'{BENCHMARK_DATA_BASE_DIR}/p1/output_{metric}_{dataset}.txt', 'r') as f1, open(f'{BENCHMARK_DATA_BASE_DIR}/p2/output_{metric}_{dataset}.txt', 'r') as f2:
                f0l = f0.readlines()
                f1l = f1.readlines()
                f2l = f2.readlines()
        x = []
        y = []
        x_traffic = []
        y_traffic = []
        if subp > 2:
            # reference has no one-time cost before first iteration, hence, do not test for depth 0.
            Ds = range(1, 11)
        else:
            Ds = range(11)
        print()
        print(f'{metric} on {dataset}')
        print('    D |    prep. comm.   online comm. prep. time online time          RAM  packing comm. overhead')
        for d in Ds: # depth
            # Compute traffic (preprocessing+online) analytically in bytes.
            # This allows to show communication even where benchmarks did not actually run.
            # See Tab. 4, p. 21 in the paper for number of ring elements.
            # Here, we just multiply by RING_SIZE // 8 to get number of bytes instead.
            # Note that t_ = n_ + #edges.
            # Also note that Tab. 4 reports online per party, so we take 2x that.
            if subp == 0:
                traffic = RING_SIZE // 8 * (16*t_*int(math.ceil(math.log2(n_)))+27*t_+8*t_*d + 2*(12*t_*int(math.ceil(math.log2(n_)))+17*t_+4*t_*d))
            elif subp == 1:
                traffic = RING_SIZE // 8 * (24*t_*math.ceil(math.log2(n_))+(55-1/RING_SIZE)*t_-3+1/RING_SIZE+8*t_*d + 2*(18*t_*math.ceil(math.log2(n_))+(40-2/RING_SIZE)*t_-6+2/RING_SIZE+4*t_*d))
            elif subp == 2:
                traffic = RING_SIZE // 8 * (16*t_*math.ceil(math.log2(n_))+(4-1/RING_SIZE)*n_*n_ + 2 * n_ * (t_ - n_) + 25*t_+8*t_*n_*d + 2*(12*t_*math.ceil(math.log2(n_))+(5-2/RING_SIZE)*n_*n_ + n_ * (t_ - n_) + 16*t_+4*t_*n_*d))
            elif subp == 3:
                traffic = RING_SIZE // 8 * 5 * (n_**2*(d-1))
            elif subp == 4:
                traffic = RING_SIZE // 8 * 5 * (n_**2*(d+l_-2))
            elif subp == 5:
                traffic = RING_SIZE // 8 * 5 * (n_**3*(d-1)+(2-1/RING_SIZE)*n_**2)
            x_traffic.append(d)
            y_traffic.append(traffic)
            benchmark_exists = benchmarked
            if subp > 2:
                if d - 1 >= len(f0l):
                    benchmark_exists = False
                else:
                    p0 = json.loads(f0l[d - 1])
                    p1 = json.loads(f1l[d - 1])
                    p2 = json.loads(f2l[d - 1])
            else:
                if d >= len(f0l):
                    benchmark_exists = False
                else:
                    p0 = json.loads(f0l[d])
                    p1 = json.loads(f1l[d])
                    p2 = json.loads(f2l[d])

            if benchmark_exists:
                assert p0['details']['D'] == d
                assert p1['details']['D'] == d
                assert p2['details']['D'] == d
                (comm_off, comm_on, time_off, time_on, ram) = aggregate_row(p0, p1, p2)
                print(f'{d:5} |  {(comm_off/1024/1024):10.3f}MiB {(comm_on/1024/1024):10.3f}MiB {(time_off/1000):10.3f}s {(time_on/1000):10.3f}s {(ram/1000/1000):10.3f}GB                 ', end='')
            
            # Analytical communication
            if subp > 2:
                trf = y_traffic[d - 1]
            else:
                trf = y_traffic[d]

            if benchmark_exists:
                # Actual communication
                # This is slightly different due to the following reasons:
                # - The helper always sends additional 56 bytes to synchronize vector sizes
                # - The benchmarks also reveal n_ many values to check correctness of the computed scores
                #       (we exclude opening from our benchmarks, but the difference is negligible)
                # - Using double shuffles costs 24*t_ ring elements for setup ONCE, but our implementation
                #       optimizes this away if the double shuffle is never used. Hence, our analytical
                #       analysis counts it as part of the one-time computation for iteration 0,
                #       but our implementation only starts actually doing it once if the number of
                #       iterations is >0. In contrast to the other differences, this actually decreases
                #       communication compared to the theoretical numbers.
                # - As stated in footnote 22, p. 19 in the paper, binary domain computation requires
                #       to pack bits into 32 bit integers. Our implementation does not as good as would
                #       be theoretically possible.
                #       Theoretically, EQZ requires 5-5/32 ring elements (32 bits) of communication.
                #       We instead need 25 ring elements with the suboptimal packing.
                #       Implementation uses 11 full ring elements for EQZ (2 and the AND) instead of 2-1/RING_SIZE ones.
                #       Furthermore, one AND costs 5 ring elements instead of 5 bits, resulting from naive packing into one 32 bit integer.
                # ==> Take this differences into account here to verify that communication is as to
                #       be expected
                tis = comm_off + 2 * comm_on
                tshould = trf + 56 + 8 * n_ # compensate 56 byte synchronization and 8*n_ byte for opening
                diff = (tis - tshould) / tshould
                if abs(diff) > 0:
                    if subp == 0 and d == 0:
                        assert tis + 24 * t_ == tshould # double shuffle setup skipped if zero iterations
                        print("   None")
                    elif subp == 1 and d > 0:
                        # Deduplication uses 2*(t_-1) EQZ and t_-1 ANDs, all with suboptimal packing
                        assert tis == tshould + RING_SIZE // 8 * (2*(t_-1) * (25 - 5 + 5/RING_SIZE) + (t_-1) * (5 - 5/RING_SIZE))
                        print(f'{(tis - tshould) / tshould * 100:6.2f}%')
                    elif subp == 1:
                        # Previous case + double shuffle setup skipped if zero iterations
                        assert tis + 24 * t_ == tshould + RING_SIZE // 8 * (2*(t_-1) * (25 - 5 + 5/RING_SIZE) + (t_-1) * (5 - 5/RING_SIZE))
                        print(f'{(tis + 24 * t_ - tshould) / tshould * 100:6.2f}%')
                    elif subp == 2 and d > 0:
                        # Final clipping uses n_^2 EQZ with suboptimal packing
                        assert tis == tshould + RING_SIZE // 8 * n_**2 * (25 - 5 + 5 / RING_SIZE)
                        print(f'{(tis - tshould) / tshould * 100:6.2f}%')
                    elif subp == 2:
                        # Previous case + double shuffle setup skipped if zero iterations
                        assert tis + 24 * t_ == tshould + RING_SIZE // 8 * n_**2 * (25 - 5 + 5 / RING_SIZE)
                        print(f'{(tis + 24 * t_ - tshould) / tshould * 100:6.2f}%')
                    elif subp == 5:
                        # Clipping uses n_^2 EQZ with suboptimal packing
                        assert tis == tshould + RING_SIZE // 8 * n_**2 * (25 - 5 + 5 / RING_SIZE)
                        print(f'{(tis - tshould) / tshould * 100:6.2f}%')
                    else:
                        assert False
                else:
                    print("   None")
                x.append(d)
                y.append((time_off + time_on) / 1000) # ms -> s

        # Configure y-axis min and max
        if di == 0:
            # run time:
            ax[di, 0].set_ylim([0, 0.5])
            # traffic:
            ax[di, 1].set_yscale('log')
            y_traffic = [x / 1024 / 1024 for x in y_traffic]
            unit = "MiB"
        elif di == 1:
            ax[di, 0].set_ylim([0, 3])
            ax[di, 1].set_yscale('log')
            y_traffic = [x / 1024 / 1024 for x in y_traffic]
            unit = "MiB"
        elif di == 2:
            ax[di, 0].set_ylim([0, 10])
            ax[di, 1].set_ylim([5, 1000])
            ax[di, 1].set_yscale('log')
            y_traffic = [x / 1024 / 1024 for x in y_traffic]
            unit = "MiB"
        elif di == 3:
            ax[di, 0].set_ylim([0, 3])
            ax[di, 1].set_ylim([0.05, 50])
            ax[di, 1].set_yscale('log')
            y_traffic = [x / 1024 / 1024 / 1024 for x in y_traffic]
            unit = "GiB"
        elif di == 4:
            ax[di, 0].set_ylim([0, 150])
            ax[di, 1].set_ylim([3, 50000])
            ax[di, 1].set_yscale('log')
            y_traffic = [x / 1024 / 1024 / 1024 for x in y_traffic]
            unit = "GiB"

        # Configure color, linestyle, x-axis, labels, etc. and plot
        if subp == 0 or subp == 3:
            col = C1
        elif subp == 1 or subp == 4:
            col = C2
        else:
            col = C3
        if subp < 3:
            lnstyle = "solid"
        else:
            lnstyle = "dotted"
        ax[di, 0].set_xlim([0,10])
        ax[di, 1].set_xlim([0,10])
        if di == 4:
            ax[di, 0].set_xlabel("                                      D",labelpad=0, fontsize=11)
            ax[di, 1].set_xlabel("D",labelpad=0, fontsize=11)
        ax[di, 0].set_ylabel("time [s]",labelpad=0, fontsize=9)
        ax[di, 0].tick_params(axis='both', which='major', labelsize=9)
        ax[di, 0].set_xticks([0,2,4,6,8,10])
        ax[di, 1].set_ylabel(f"comm. [{unit}]",labelpad=0, fontsize=9)
        ax[di, 1].tick_params(axis='both', which='major', labelsize=9)
        ax[di, 1].set_xticks([0,2,4,6,8,10])
        plt.rcParams['axes.titley'] = 1.0
        plt.rcParams['axes.titlepad'] = -12
        if di == 0:
            if subp == 0:
                lbl = r'$\pi_\mathsf{M}^D$ ($\bf{ours}$)'
            elif subp == 1:
                lbl = r'$\pi_\mathsf{K}^D$ ($\bf{ours}$)'
            elif subp == 2:
                lbl = r'$\pi_\mathsf{R}^D$ ($\bf{ours}$)'
            elif subp == 3:
                lbl = r'$\pi_\mathsf{M}^D$ (prior [6])'
            elif subp == 4:
                lbl = r'$\pi_\mathsf{K}^D$ (prior [6])'
            elif subp == 5:
                lbl = r'$\pi_\mathsf{R}^D$ (prior [6])'
            else:
                lbl = 'ERROR'
            ax[di, 0].plot(x, y, color=col, linestyle=lnstyle, marker=".", label=lbl)
        else:
            ax[di, 0].plot(x, y, color=col, linestyle=lnstyle, marker=".")
        
        # ax[di, 1].plot(x, y, color=col, linestyle=lnstyle, marker=".")
        ax[di, 1].plot(x_traffic, y_traffic, color=col, linestyle=lnstyle)

# final formatting
fig.align_ylabels(ax[:, 0])
fig.align_ylabels(ax[:, 1])
fig.set_size_inches(6, 7)
fig.get_layout_engine().set(w_pad = 1 / 72, h_pad = 2 / 72, hspace=0,
                            wspace=0)
fig.get_layout_engine().set(rect=(0.17,0.05,0.83,0.95))
lines_labels = [ax.get_legend_handles_labels() for ax in fig.axes]
lines, labels = [sum(lol, []) for lol in zip(*lines_labels)]
fig.legend(lines, labels, loc=(0,0), ncol=2, prop={'size': 11.5},framealpha=1)
fig.text(0.04, 1 - 0.5 * (0.943) / 5, "aarhus", fontsize=12)
fig.text(0.04, 1 - 1.5 * (0.943) / 5, "london", fontsize=12)
fig.text(0.04, 1 - 2.5 * (0.943) / 5, "hiv", fontsize=12)
fig.text(0.04, 1 - 3.5 * (0.943) / 5, "arabi", fontsize=12)
fig.text(0.04, 1 - 4.5 * (0.943) / 5, "higgs", fontsize=12)
rc('pdf', fonttype=42)
plt.savefig('dataset_plots.pdf')
plt.show()
