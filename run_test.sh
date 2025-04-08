# Expect as $1 the pid and as $2 the test case

if [ $2 = 0 ]; then
    set -o xtrace
    ./test --localhost --pid $1
elif [ $2 = 1 ]; then
    set -o xtrace
    ./shuffle --localhost --vec-size 10 --pid $1
elif [ $2 = 2 ]; then
    set -o xtrace
    ./doubleshuffle --localhost --vec-size 10 --pid $1
elif [ $2 = 3 ]; then
    set -o xtrace
    ./compaction --localhost --vec-size 10 --pid $1
elif [ $2 = 4 ]; then
    set -o xtrace
    ./sort --localhost --vec-size 10 --pid $1
elif [ $2 = 5 ]; then
    set -o xtrace
    ./equalszero --localhost --pid $1
elif [ $2 = 6 ]; then
    set -o xtrace
    ./pi_3_test --localhost --pid $1
elif [ $2 = 7 ]; then
    set -o xtrace
    ./pi_3_benchmark --localhost --depth 0 --nodes 10 --size 20 --pid $1
elif [ $2 = 8 ]; then
    set -o xtrace
    ./pi_3_benchmark --localhost --depth 1 --nodes 10 --size 20 --pid $1
elif [ $2 = 9 ]; then
    set -o xtrace
    ./pi_3_benchmark --localhost --depth 2 --nodes 10 --size 20 --pid $1
elif [ $2 = 10 ]; then
    set -o xtrace
    ./pi_3_ref_test --localhost --pid $1
elif [ $2 = 11 ]; then
    set -o xtrace
    ./pi_3_ref_benchmark --localhost --depth 1 --nodes 10 --layers 3 --pid $1
elif [ $2 = 12 ]; then
    set -o xtrace
    ./pi_3_ref_benchmark --localhost --depth 2 --nodes 10 --layers 3 --pid $1
elif [ $2 = 13 ]; then
    set -o xtrace
    ./pi_2_test --localhost --pid $1
elif [ $2 = 14 ]; then
    set -o xtrace
    ./pi_2_benchmark --localhost --depth 0 --nodes 10 --size 20 --pid $1
elif [ $2 = 15 ]; then
    set -o xtrace
    ./pi_2_benchmark --localhost --depth 1 --nodes 10 --size 20 --pid $1
elif [ $2 = 16 ]; then
    set -o xtrace
    ./pi_2_benchmark --localhost --depth 2 --nodes 10 --size 20 --pid $1
elif [ $2 = 17 ]; then
    set -o xtrace
    ./pi_2_ref_test --localhost --pid $1
elif [ $2 = 18 ]; then
    set -o xtrace
    ./pi_2_ref_benchmark --localhost --depth 1 --nodes 10 --layers 3 --pid $1
elif [ $2 = 19 ]; then
    set -o xtrace
    ./pi_2_ref_benchmark --localhost --depth 2 --nodes 10 --layers 3 --pid $1
elif [ $2 = 20 ]; then
    set -o xtrace
    ./pi_1_test --localhost --pid $1
elif [ $2 = 21 ]; then
    set -o xtrace
    ./pi_1_benchmark --localhost --depth 0 --nodes 10 --size 20 --pid $1
elif [ $2 = 22 ]; then
    set -o xtrace
    ./pi_1_benchmark --localhost --depth 1 --nodes 10 --size 20 --pid $1
elif [ $2 = 23 ]; then
    set -o xtrace
    ./pi_1_benchmark --localhost --depth 2 --nodes 10 --size 20 --pid $1
elif [ $2 = 24 ]; then
    set -o xtrace
    ./pi_1_ref_test --localhost --pid $1
elif [ $2 = 25 ]; then
    set -o xtrace
    ./pi_1_ref_benchmark --localhost --depth 1 --nodes 10 --layers 3 --pid $1
elif [ $2 = 26 ]; then
    set -o xtrace
    ./pi_1_ref_benchmark --localhost --depth 2 --nodes 10 --layers 3 --pid $1
else
    echo "unknown test case"
fi