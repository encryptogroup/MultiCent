#!/bin/bash
# set -x

dir=../Results

# rm -rf $dir/*.log $dir/g*.json
rm -rf $dir
mkdir -p $dir
graph_size="10000"


for players in 2
do
    for rounds in 1
    do
        for party in $(seq 1 $players)
        do
            logdir=$dir/$players\_PC/$vec_size\_Nodes/TestRun/
            mkdir -p $logdir
            log=$logdir/round\_$rounds\_party\_$party.log
            tplog=$logdir/round\_$rounds\_party\_0.log
            # graphsc_linear
            # graphsc_ro
            # graphiti_mpa
            # applyV
            if [ $party -eq 1 ]; then
                ./benchmarks/applyV -p $party --localhost -v $graph_size 2>&1 | cat > $log &
            else
                ./benchmarks/applyV -p $party --localhost -v $graph_size 2>&1 | cat > $log &
            fi
            codes[$party]=$!
        done

        ./benchmarks/applyV -p 0 --localhost -v $graph_size 2>&1 | cat > $tplog &
        codes[0]=$!
        for party in $(seq 0 $players)
        do
            wait ${codes[$party]} || return 1
        done
    done
done

