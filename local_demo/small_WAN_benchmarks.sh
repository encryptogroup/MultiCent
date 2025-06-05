set -o xtrace

BASE_PORT=11000
ALTERNATIVE_PORT=11018 # 11000 + 2 * 3 * 3
CURRENT_PORT=$BASE_PORT

rotate_ports() {
    if [ $CURRENT_PORT -eq $BASE_PORT ] ; then
        CURRENT_PORT=$ALTERNATIVE_PORT
    else
        CURRENT_PORT=$BASE_PORT
    fi
}

mkdir -p p0
mkdir -p p1
mkdir -p p2

./network_wan.sh || exit 1

run_benchmark() {
    ./$1_benchmark --pid 0 -n $2 -s $3 -d $4 -o p0/WAN_output_$1_$5.txt -r $6 --localhost --port $CURRENT_PORT > /dev/null &
    ./$1_benchmark --pid 2 -n $2 -s $3 -d $4 -o p2/WAN_output_$1_$5.txt -r $6 --localhost --port $CURRENT_PORT > /dev/null &
    ./$1_benchmark --pid 1 -n $2 -s $3 -d $4 -o p1/WAN_output_$1_$5.txt -r $6 --localhost --port $CURRENT_PORT
    rotate_ports
}

run_benchmark_ref() {
    ./$1_ref_benchmark --pid 0 -n $2 -l $3 -d $4 -o p0/WAN_output_$1_ref_$5.txt -r $6 --localhost --port $CURRENT_PORT > /dev/null &
    ./$1_ref_benchmark --pid 2 -n $2 -l $3 -d $4 -o p2/WAN_output_$1_ref_$5.txt -r $6 --localhost --port $CURRENT_PORT > /dev/null &
    ./$1_ref_benchmark --pid 1 -n $2 -l $3 -d $4 -o p1/WAN_output_$1_ref_$5.txt -r $6 --localhost --port $CURRENT_PORT
    rotate_ports
}

#################################
### Scalability for MultiCent ###
#################################

# Using s = 11*n and 51*n and 101*n, i.e., #edges = 10* or 50* or 100* #nodes

# pi 3
for i in 0 1 2; do
for n in 100 200 300 400 500 600 700 800 900 1000 2000 3000 4000 5000 6000 7000 8000 9000 10000; do
    # OMITTED: 20k, 30k, 40k, 50k
    run_benchmark pi_3 $n $((11*n)) $i 11_d$i 1
done
for n in 100 200 300 400 500 600 700 800 900 1000 2000 3000 4000; do
    # OMITTED: 5k, 6k, 7k, 8k, 9k, 10k, 20k, 30k, 40k, 50k
    run_benchmark pi_3 $n $((51*n)) $i 51_d$i 1
done
for n in 100 200 300 400 500 600 700 800 900 1000 2000; do
    # OMITTED: 3k, 4k, 5k, 6k, 7k, 8k, 9k, 10k, 20k, 30k, 40k, 50k
    run_benchmark pi_3 $n $((101*n)) $i 101_d$i 1
done
done


#################################
### Scalability for Reference ###
#################################

# pi 3
for i in 1 2; do
for n in 100 200 300 400 500 600 700 800 900 1000; do
    # OMITTED: 2k, 3k, 4k, 5k, 6k, 7k, 8k
    run_benchmark_ref pi_3 $n 3 $i d$i 1
done
done

./network_off.sh || exit 1
