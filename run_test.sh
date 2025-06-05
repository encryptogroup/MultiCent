# Expect as $1 the test case number

if [ $1 = 0 ]; then
    set -o xtrace
    ./test --localhost --pid 0 > /dev/null &
    ./test --localhost --pid 2 > /dev/null &
    ./test --localhost --pid 1
elif [ $1 = 1 ]; then
    set -o xtrace
    ./shuffle --localhost --vec-size 10 --pid 0 > /dev/null &
    ./shuffle --localhost --vec-size 10 --pid 2 > /dev/null &
    ./shuffle --localhost --vec-size 10 --pid 1
elif [ $1 = 2 ]; then
    set -o xtrace
    ./doubleshuffle --localhost --vec-size 10 --pid 0 > /dev/null &
    ./doubleshuffle --localhost --vec-size 10 --pid 2 > /dev/null &
    ./doubleshuffle --localhost --vec-size 10 --pid 1
elif [ $1 = 3 ]; then
    set -o xtrace
    ./compaction --localhost --vec-size 10 --pid 0 > /dev/null &
    ./compaction --localhost --vec-size 10 --pid 2 > /dev/null &
    ./compaction --localhost --vec-size 10 --pid 1
elif [ $1 = 4 ]; then
    set -o xtrace
    ./sort --localhost --vec-size 10 --pid 0 > /dev/null &
    ./sort --localhost --vec-size 10 --pid 2 > /dev/null &
    ./sort --localhost --vec-size 10 --pid 1
elif [ $1 = 5 ]; then
    set -o xtrace
    ./equalszero --localhost --pid 0 > /dev/null &
    ./equalszero --localhost --pid 2 > /dev/null &
    ./equalszero --localhost --pid 1
elif [ $1 = 6 ]; then
    set -o xtrace
    ./pi_3_test --localhost --pid 0 > /dev/null &
    ./pi_3_test --localhost --pid 2 > /dev/null &
    ./pi_3_test --localhost --pid 1
elif [ $1 = 7 ]; then
    set -o xtrace
    ./pi_3_benchmark --localhost --depth 0 --nodes 10 --size 20 --pid 0 > /dev/null &
    ./pi_3_benchmark --localhost --depth 0 --nodes 10 --size 20 --pid 2 > /dev/null &
    ./pi_3_benchmark --localhost --depth 0 --nodes 10 --size 20 --pid 1
elif [ $1 = 8 ]; then
    set -o xtrace
    ./pi_3_benchmark --localhost --depth 1 --nodes 10 --size 20 --pid 0 > /dev/null &
    ./pi_3_benchmark --localhost --depth 1 --nodes 10 --size 20 --pid 2 > /dev/null &
    ./pi_3_benchmark --localhost --depth 1 --nodes 10 --size 20 --pid 1
elif [ $1 = 9 ]; then
    set -o xtrace
    ./pi_3_benchmark --localhost --depth 2 --nodes 10 --size 20 --pid 0 > /dev/null &
    ./pi_3_benchmark --localhost --depth 2 --nodes 10 --size 20 --pid 2 > /dev/null &
    ./pi_3_benchmark --localhost --depth 2 --nodes 10 --size 20 --pid 1
elif [ $1 = 10 ]; then
    set -o xtrace
    ./pi_3_ref_test --localhost --pid 0 > /dev/null &
    ./pi_3_ref_test --localhost --pid 2 > /dev/null &
    ./pi_3_ref_test --localhost --pid 1
elif [ $1 = 11 ]; then
    set -o xtrace
    ./pi_3_ref_benchmark --localhost --depth 1 --nodes 10 --layers 3 --pid 0 > /dev/null &
    ./pi_3_ref_benchmark --localhost --depth 1 --nodes 10 --layers 3 --pid 2 > /dev/null &
    ./pi_3_ref_benchmark --localhost --depth 1 --nodes 10 --layers 3 --pid 1
elif [ $1 = 12 ]; then
    set -o xtrace
    ./pi_3_ref_benchmark --localhost --depth 2 --nodes 10 --layers 3 --pid 0 > /dev/null &
    ./pi_3_ref_benchmark --localhost --depth 2 --nodes 10 --layers 3 --pid 2 > /dev/null &
    ./pi_3_ref_benchmark --localhost --depth 2 --nodes 10 --layers 3 --pid 1
elif [ $1 = 13 ]; then
    set -o xtrace
    ./pi_2_test --localhost --pid 0 > /dev/null &
    ./pi_2_test --localhost --pid 2 > /dev/null &
    ./pi_2_test --localhost --pid 1
elif [ $1 = 14 ]; then
    set -o xtrace
    ./pi_2_benchmark --localhost --depth 0 --nodes 10 --size 20 --pid 0 > /dev/null &
    ./pi_2_benchmark --localhost --depth 0 --nodes 10 --size 20 --pid 2 > /dev/null &
    ./pi_2_benchmark --localhost --depth 0 --nodes 10 --size 20 --pid 1
elif [ $1 = 15 ]; then
    set -o xtrace
    ./pi_2_benchmark --localhost --depth 1 --nodes 10 --size 20 --pid 0 > /dev/null &
    ./pi_2_benchmark --localhost --depth 1 --nodes 10 --size 20 --pid 2 > /dev/null &
    ./pi_2_benchmark --localhost --depth 1 --nodes 10 --size 20 --pid 1
elif [ $1 = 16 ]; then
    set -o xtrace
    ./pi_2_benchmark --localhost --depth 2 --nodes 10 --size 20 --pid 0 > /dev/null &
    ./pi_2_benchmark --localhost --depth 2 --nodes 10 --size 20 --pid 2 > /dev/null &
    ./pi_2_benchmark --localhost --depth 2 --nodes 10 --size 20 --pid 1
elif [ $1 = 17 ]; then
    set -o xtrace
    ./pi_2_ref_test --localhost --pid 0 > /dev/null &
    ./pi_2_ref_test --localhost --pid 2 > /dev/null &
    ./pi_2_ref_test --localhost --pid 1
elif [ $1 = 18 ]; then
    set -o xtrace
    ./pi_2_ref_benchmark --localhost --depth 1 --nodes 10 --layers 3 --pid 0 > /dev/null &
    ./pi_2_ref_benchmark --localhost --depth 1 --nodes 10 --layers 3 --pid 2 > /dev/null &
    ./pi_2_ref_benchmark --localhost --depth 1 --nodes 10 --layers 3 --pid 1
elif [ $1 = 19 ]; then
    set -o xtrace
    ./pi_2_ref_benchmark --localhost --depth 2 --nodes 10 --layers 3 --pid 0 > /dev/null &
    ./pi_2_ref_benchmark --localhost --depth 2 --nodes 10 --layers 3 --pid 2 > /dev/null &
    ./pi_2_ref_benchmark --localhost --depth 2 --nodes 10 --layers 3 --pid 1
elif [ $1 = 20 ]; then
    set -o xtrace
    ./pi_1_test --localhost --pid 0 > /dev/null &
    ./pi_1_test --localhost --pid 2 > /dev/null &
    ./pi_1_test --localhost --pid 1
elif [ $1 = 21 ]; then
    set -o xtrace
    ./pi_1_benchmark --localhost --depth 0 --nodes 10 --size 20 --pid 0 > /dev/null &
    ./pi_1_benchmark --localhost --depth 0 --nodes 10 --size 20 --pid 2 > /dev/null &
    ./pi_1_benchmark --localhost --depth 0 --nodes 10 --size 20 --pid 1
elif [ $1 = 22 ]; then
    set -o xtrace
    ./pi_1_benchmark --localhost --depth 1 --nodes 10 --size 20 --pid 0 > /dev/null &
    ./pi_1_benchmark --localhost --depth 1 --nodes 10 --size 20 --pid 2 > /dev/null &
    ./pi_1_benchmark --localhost --depth 1 --nodes 10 --size 20 --pid 1
elif [ $1 = 23 ]; then
    set -o xtrace
    ./pi_1_benchmark --localhost --depth 2 --nodes 10 --size 20 --pid 0 > /dev/null &
    ./pi_1_benchmark --localhost --depth 2 --nodes 10 --size 20 --pid 2 > /dev/null &
    ./pi_1_benchmark --localhost --depth 2 --nodes 10 --size 20 --pid 1
elif [ $1 = 24 ]; then
    set -o xtrace
    ./pi_1_ref_test --localhost --pid 0 > /dev/null &
    ./pi_1_ref_test --localhost --pid 2 > /dev/null &
    ./pi_1_ref_test --localhost --pid 1
elif [ $1 = 25 ]; then
    set -o xtrace
    ./pi_1_ref_benchmark --localhost --depth 1 --nodes 10 --layers 3 --pid 0 > /dev/null &
    ./pi_1_ref_benchmark --localhost --depth 1 --nodes 10 --layers 3 --pid 2 > /dev/null &
    ./pi_1_ref_benchmark --localhost --depth 1 --nodes 10 --layers 3 --pid 1
elif [ $1 = 26 ]; then
    set -o xtrace
    ./pi_1_ref_benchmark --localhost --depth 2 --nodes 10 --layers 3 --pid 0 > /dev/null &
    ./pi_1_ref_benchmark --localhost --depth 2 --nodes 10 --layers 3 --pid 2 > /dev/null &
    ./pi_1_ref_benchmark --localhost --depth 2 --nodes 10 --layers 3 --pid 1
else
    echo "unknown test case"
fi