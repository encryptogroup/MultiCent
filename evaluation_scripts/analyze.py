import sys
import json
import statistics

if len(sys.argv) < 2:
    exit("Missing target. Options: datasets_small/datasets_large/scalability_pi3/scalability_pi2/scalability_pi1/scalability_pi3_WAN")

if sys.argv[1] == "datasets_small":
    # Tables: aarhus/london/hiv x pi3/pi2/pi1
    # Columns: D, our cost and WWW'17 cost, both with preproc comm, online comm, preproc time, online time, memory
    TABLES = [  ("output_pi_3_§aarhus", "pi3 aarhus"), ("output_pi_2_§aarhus", "pi2 aarhus"), ("output_pi_1_§aarhus", "pi1 aarhus"),\
                ("output_pi_3_§london", "pi3 london"), ("output_pi_2_§london", "pi2 london"), ("output_pi_1_§london", "pi1 london"),\
                ("output_pi_3_§hiv", "pi3 hiv"), ("output_pi_2_§hiv", "pi2 hiv"), ("output_pi_1_§hiv", "pi1 hiv")]
    MODE = 1 # ours vs reference instead of one-time vs per iteration
    MAX_D = 10
elif sys.argv[1] == "datasets_large":
    # Tables: aarhus/london/hiv x pi3/pi2
    # Columns: D, our cost, with preproc comm, online comm, preproc time, online time, memory
    TABLES = [  ("output_pi_3_arabi", "pi3 arabi"), ("output_pi_2_arabi", "pi2 arabi"),\
                ("output_pi_3_higgs", "pi3 higgs"), ("output_pi_2_higgs", "pi2 higgs")]
    MODE = 2 # only ours instead of ours vs reference or one-time vs per iteration
    MAX_D = 10
elif sys.argv[1] == "scalability_pi3":
    # Tables: E=10V, E=50V, E=100V, reference
    # Columns: V, one-time cost and cost per iteration, both with preproc comm, online comm, preproc time, online time, memory
    TABLES = [("output_pi_3_11", "|E|=10*|V|"), ("output_pi_3_51", "|E|=50*|V|"), ("output_pi_3_101", "|E|=100*|V|"), ("output_pi_3_ref", "WWW'17 reference")]
    MODE = 0
elif sys.argv[1] == "scalability_pi2":
    # Tables: E=10V, E=50V, E=100V, reference
    # Columns: V, one-time cost and cost per iteration, both with preproc comm, online comm, preproc time, online time, memory
    TABLES = [("output_pi_2_11", "|E|=10*|V|"), ("output_pi_2_51", "|E|=50*|V|"), ("output_pi_2_101", "|E|=100*|V|"), ("output_pi_2_ref", "WWW'17 reference")]
    MODE = 0
elif sys.argv[1] == "scalability_pi1":
    # Tables: E=10V, E=50V, E=100V, reference
    # Columns: V, one-time cost and cost per iteration, both with preproc comm, online comm, preproc time, online time, memory
    TABLES = [("output_pi_1_11", "|E|=10*|V|"), ("output_pi_1_51", "|E|=50*|V|"), ("output_pi_1_101", "|E|=100*|V|"), ("output_pi_1_ref", "WWW'17 reference")]
    MODE = 0
elif sys.argv[1] == "scalability_pi3_WAN":
    # Tables: E=10V, E=50V, E=100V, reference
    # Columns: V, one-time cost and cost per iteration, both with preproc comm, online comm, preproc time, online time, memory
    TABLES = [("WAN_output_pi_3_11", "|E|=10*|V|"), ("WAN_output_pi_3_51", "|E|=50*|V|"), ("WAN_output_pi_3_101", "|E|=100*|V|"), ("WAN_output_pi_3_ref", "WWW'17 reference")]
    MODE = 0
else:
    exit(f"Invalid target '{sys.argv[1]}'. Options: datasets_small/datasets_large/scalability_pi3/scalability_pi2/scalability_pi1/scalability_pi3_WAN")

if "original" in sys.argv:
    BENCHMARK_DATA_BASE_DIR = "raw_benchmark_outputs"
else:
    BENCHMARK_DATA_BASE_DIR = "../build/benchmarks"

METRICS = ["pi_3", "pi_2", "pi_1"]
MAX_R = 3
Ns = [100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000, 20000, 30000, 40000, 50000, 60000, 70000, 80000, 90000, 100000, 200000, 300000, 400000, 500000]
M_FACTORS = [10, 50, 100]

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

if __name__ == "__main__":
    for table, title in TABLES:
        print(f'\n\n### {title}')
        if MODE == 0:
            print("       |                      one-time cost                    |                    cost per iteration                ")
            print("   |V| |   pre comm   onl comm   pre time   onl time       RAM |   pre comm   onl comm   pre time   onl time       RAM")
            if "ref" not in table:
                with    open(f'{BENCHMARK_DATA_BASE_DIR}/p0/{table}_d0.txt', 'r') as f0d0, open(f'{BENCHMARK_DATA_BASE_DIR}/p1/{table}_d0.txt', 'r') as f1d0, open(f'{BENCHMARK_DATA_BASE_DIR}/p2/{table}_d0.txt', 'r') as f2d0:
                    f0d0l = f0d0.readlines()
                    f1d0l = f1d0.readlines()
                    f2d0l = f2d0.readlines()
            with    open(f'{BENCHMARK_DATA_BASE_DIR}/p0/{table}_d1.txt', 'r') as f0d1, open(f'{BENCHMARK_DATA_BASE_DIR}/p1/{table}_d1.txt', 'r') as f1d1, open(f'{BENCHMARK_DATA_BASE_DIR}/p2/{table}_d1.txt', 'r') as f2d1, \
                    open(f'{BENCHMARK_DATA_BASE_DIR}/p0/{table}_d2.txt', 'r') as f0d2, open(f'{BENCHMARK_DATA_BASE_DIR}/p1/{table}_d2.txt', 'r') as f1d2, open(f'{BENCHMARK_DATA_BASE_DIR}/p2/{table}_d2.txt', 'r') as f2d2:
                f0d1l = f0d1.readlines()
                f1d1l = f1d1.readlines()
                f2d1l = f2d1.readlines()
                f0d2l = f0d2.readlines()
                f1d2l = f1d2.readlines()
                f2d2l = f2d2.readlines()
            i = 0
            while i < max(len(f0d1l),len(f1d1l),len(f2d1l),len(f0d2l),len(f1d2l),len(f2d2l)):
                if "ref" not in table:
                    p0d0 = json.loads(f0d0l[i])
                    p1d0 = json.loads(f1d0l[i])
                    p2d0 = json.loads(f2d0l[i])
                p0d1 = json.loads(f0d1l[i])
                p1d1 = json.loads(f1d1l[i])
                p2d1 = json.loads(f2d1l[i])
                p0d2 = json.loads(f0d2l[i])
                p1d2 = json.loads(f1d2l[i])
                p2d2 = json.loads(f2d2l[i])
                i += 1

                if "ref" not in table:
                    assert p0d0['details']['D'] == 0
                    assert p1d0['details']['D'] == 0
                    assert p2d0['details']['D'] == 0
                    assert p0d0['details']['pid'] == 0
                    assert p1d0['details']['pid'] == 1
                    assert p2d0['details']['pid'] == 2
                assert p0d1['details']['D'] == 1
                assert p1d1['details']['D'] == 1
                assert p2d1['details']['D'] == 1
                assert p0d2['details']['D'] == 2
                assert p1d2['details']['D'] == 2
                assert p2d2['details']['D'] == 2
                assert p0d1['details']['pid'] == 0
                assert p1d1['details']['pid'] == 1
                assert p2d1['details']['pid'] == 2
                assert p0d2['details']['pid'] == 0
                assert p1d2['details']['pid'] == 1
                assert p2d2['details']['pid'] == 2

                # For ref, read data for D=2 and D=1.
                # For ours, read data for D=2, D=1, and D=0.
                # Stragegy after that:
                # Cost for one reference iteration is cost for D=2 minus cost for D=1.
                # Run time cost for one of our iterations is cost for D=2 minus cost for D=0, then divided by 2.
                # This is relevant as our iterations are relatively cheap so that averaging over 2 iterations
                # compensates some statistical fluctuation.
                # For communication, we take cost for D=2 minus cost for D=1 as there is no fluctuation.
                # Also, convert time to s, communication to MiB, and RAM to GB
                if "ref" not in table:
                    res_d0 = aggregate_row(p0d0, p1d0, p2d0)
                else:
                    res_d0 = None
                res_d1 = aggregate_row(p0d1, p1d1, p2d1)
                res_d2 = aggregate_row(p0d2, p1d2, p2d2)
                if "ref" not in table:
                    n = res_d0[0]
                    assert n == res_d1[0] and n == res_d2[0]
                else:
                    n = res_d1[0]
                    assert n == res_d2[0]

                if "ref" not in table:
                    ot_comm_off = res_d0[1] / 1024 / 1024
                    ot_comm_on = res_d0[2] / 1024 / 1024
                    ot_time_off = res_d0[3] / 1000
                    ot_time_on = res_d0[4] / 1000
                    ot_ram = res_d0[5] / 1000 / 1000
                    pi_comm_off = (res_d2[1] - res_d1[1]) / 1024 / 1024
                    pi_comm_on = (res_d2[2] - res_d1[2]) / 1024 / 1024
                    pi_time_off = (res_d2[3] - res_d0[3]) / 2 / 1000
                    pi_time_on = (res_d2[4] - res_d0[4]) / 2 / 1000
                    pi_ram = (res_d2[5] - res_d0[5]) / 2 / 1000 / 1000
                else:
                    ot_comm_off = res_d1[1] / 1024 / 1024
                    ot_comm_on = res_d1[2] / 1024 / 1024
                    ot_time_off = res_d1[3] / 1000
                    ot_time_on = res_d1[4] / 1000
                    ot_ram = res_d1[5] / 1000 / 1000
                    pi_comm_off = (res_d2[1] - res_d1[1]) / 1024 / 1024
                    pi_comm_on = (res_d2[2] - res_d1[2]) / 1024 / 1024
                    pi_time_off = (res_d2[3] - res_d1[3]) / 1000
                    pi_time_on = (res_d2[4] - res_d1[4]) / 1000
                    pi_ram = (res_d2[5] - res_d1[5]) / 1000 / 1000

                print(f'{n:6} | {ot_comm_off:7.2f}MiB {ot_comm_on:7.2f}MiB   {ot_time_off:7.3f}s   {ot_time_on:7.3f}s   {ot_ram:5.1f}GB | {pi_comm_off:7.2f}MiB {pi_comm_on:7.2f}MiB   {pi_time_off:7.3f}s   {pi_time_on:7.3f}s   {pi_ram:5.1f}GB')

        elif MODE == 1:

            print("    |                      our protocol                     |                    WWW'17 reference                  ")
            print("  d |   pre comm   onl comm   pre time   onl time       RAM |   pre comm   onl comm   pre time   onl time       RAM")
            with    open(f'{BENCHMARK_DATA_BASE_DIR}/p0/{table.replace("§","")}.txt', 'r') as f0, open(f'{BENCHMARK_DATA_BASE_DIR}/p1/{table.replace("§","")}.txt', 'r') as f1, open(f'{BENCHMARK_DATA_BASE_DIR}/p2/{table.replace("§","")}.txt', 'r') as f2, \
                    open(f'{BENCHMARK_DATA_BASE_DIR}/p0/{table.replace("§","ref_")}.txt', 'r') as f0ref, open(f'{BENCHMARK_DATA_BASE_DIR}/p1/{table.replace("§","ref_")}.txt', 'r') as f1ref, open(f'{BENCHMARK_DATA_BASE_DIR}/p2/{table.replace("§","ref_")}.txt', 'r') as f2ref:
                f0 = f0.readlines()
                f1 = f1.readlines()
                f2 = f2.readlines()
                f0ref = f0ref.readlines()
                f1ref = f1ref.readlines()
                f2ref = f2ref.readlines()
            for d in range(MAX_D + 1):
                if d >= len(f0):
                    p0 = None
                    p1 = None
                    p2 = None
                else:
                    p0 = json.loads(f0[d])
                    p1 = json.loads(f1[d])
                    p2 = json.loads(f2[d])
                    assert p0['details']['D'] == d
                    assert p1['details']['D'] == d
                    assert p2['details']['D'] == d
                    assert p0['details']['pid'] == 0
                    assert p1['details']['pid'] == 1
                    assert p2['details']['pid'] == 2
                if d == 0 or d - 1 >= len(f0ref):
                    p0ref = None
                    p1ref = None
                    p2ref = None
                else:
                    p0ref = json.loads(f0ref[d - 1]) # no d=0 for ref
                    p1ref = json.loads(f1ref[d - 1]) # no d=0 for ref
                    p2ref = json.loads(f2ref[d - 1]) # no d=0 for ref
                    assert p0ref['details']['D'] == d
                    assert p1ref['details']['D'] == d
                    assert p2ref['details']['D'] == d
                    assert p0ref['details']['pid'] == 0
                    assert p1ref['details']['pid'] == 1
                    assert p2ref['details']['pid'] == 2

                if p0 is not None:
                    res = aggregate_row(p0, p1, p2)
                    comm_off = res[1] / 1024 / 1024
                    comm_on = res[2] / 1024 / 1024
                    time_off = res[3] / 1000
                    time_on = res[4] / 1000
                    ram = res[5] / 1000 / 1000
                    own_columns = f'{comm_off:7.2f}MiB {comm_on:7.2f}MiB   {time_off:7.3f}s   {time_on:7.3f}s   {ram:5.1f}GB'
                else:
                    own_columns = '                                                      '
                if p0ref is not None:
                    res_ref = aggregate_row(p0ref, p1ref, p2ref)
                    comm_off = res_ref[1] / 1024 / 1024
                    comm_on = res_ref[2] / 1024 / 1024
                    time_off = res_ref[3] / 1000
                    time_on = res_ref[4] / 1000
                    ram = res_ref[5] / 1000 / 1000
                    ref_columns = f'{comm_off:7.2f}MiB {comm_on:7.2f}MiB   {time_off:7.3f}s   {time_on:7.3f}s   {ram:5.1f}GB'
                else:
                    ref_columns = '                                                      '

                print(f' {d:2} | {own_columns} | {ref_columns}')


        elif MODE == 2:

            print("    |                      our protocol                     ")
            print("  d |   pre comm   onl comm   pre time   onl time       RAM ")
            with    open(f'{BENCHMARK_DATA_BASE_DIR}/p0/{table}.txt', 'r') as f0, open(f'{BENCHMARK_DATA_BASE_DIR}/p1/{table}.txt', 'r') as f1, open(f'{BENCHMARK_DATA_BASE_DIR}/p2/{table.replace("§","")}.txt', 'r') as f2:
                f0 = f0.readlines()
                f1 = f1.readlines()
                f2 = f2.readlines()
            for d in range(MAX_D + 1):
                if d >= len(f0):
                    p0 = None
                    p1 = None
                    p2 = None
                else:
                    p0 = json.loads(f0[d])
                    p1 = json.loads(f1[d])
                    p2 = json.loads(f2[d])
                    assert p0['details']['D'] == d
                    assert p1['details']['D'] == d
                    assert p2['details']['D'] == d
                    assert p0['details']['pid'] == 0
                    assert p1['details']['pid'] == 1
                    assert p2['details']['pid'] == 2

                if p0 is not None:
                    res = aggregate_row(p0, p1, p2)
                    comm_off = res[1] / 1024 / 1024
                    comm_on = res[2] / 1024 / 1024
                    time_off = res[3] / 1000
                    time_on = res[4] / 1000
                    ram = res[5] / 1000 / 1000
                    own_columns = f'{comm_off:7.2f}MiB {comm_on:7.2f}MiB   {time_off:7.3f}s   {time_on:7.3f}s   {ram:5.1f}GB'
                else:
                    own_columns = '                                                      '

                print(f' {d:2} | {own_columns} ')
