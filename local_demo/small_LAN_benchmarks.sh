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

# Delete potential benchmark data from prior runs that are to be produced by this script
# (otherwise, there will be duplicate data points, leading to problems when plotting)
rm -f p0/output_pi_*
rm -f p1/output_pi_*
rm -f p2/output_pi_*

./network_lan.sh || exit 1

run_benchmark() {
    ./$1_benchmark --pid 0 -n $2 -s $3 -d $4 -o p0/output_$1_$5.txt -r $6 --localhost --port $CURRENT_PORT >> p0/log.txt 2>&1 &
    ./$1_benchmark --pid 2 -n $2 -s $3 -d $4 -o p2/output_$1_$5.txt -r $6 --localhost --port $CURRENT_PORT >> p2/log.txt 2>&1 &
    ./$1_benchmark --pid 1 -n $2 -s $3 -d $4 -o p1/output_$1_$5.txt -r $6 --localhost --port $CURRENT_PORT 2>&1 | tee -a p1/log.txt
    rotate_ports
}

run_benchmark_ref() {
    ./$1_ref_benchmark --pid 0 -n $2 -l $3 -d $4 -o p0/output_$1_ref_$5.txt -r $6 --localhost --port $CURRENT_PORT >> p0/log.txt 2>&1 &
    ./$1_ref_benchmark --pid 2 -n $2 -l $3 -d $4 -o p2/output_$1_ref_$5.txt -r $6 --localhost --port $CURRENT_PORT >> p2/log.txt 2>&1 &
    ./$1_ref_benchmark --pid 1 -n $2 -l $3 -d $4 -o p1/output_$1_ref_$5.txt -r $6 --localhost --port $CURRENT_PORT 2>&1 | tee -a p1/log.txt
    rotate_ports
}

###########################
### aarhus, london, hiv ###
###########################

for i in {0..10}; do
    run_benchmark pi_3 61 1301 $i aarhus 3
    run_benchmark pi_3 369 1375 $i london 3
    run_benchmark pi_3 1005 3693 $i hiv 3
done

for i in {1..10}; do
    run_benchmark_ref pi_3 61 5 $i aarhus 3
    run_benchmark_ref pi_3 369 3 $i london 3
done
for i in {1..6}; do
    # OMITTED: i=7..10
    run_benchmark_ref pi_3 1005 3 $i hiv 3
done

for i in {0..10}; do
    run_benchmark pi_2 61 1301 $i aarhus 3
    run_benchmark pi_2 369 1375 $i london 3
    run_benchmark pi_2 1005 3693 $i hiv 3
done

for i in {1..10}; do
    run_benchmark_ref pi_2 61 5 $i aarhus 3
    run_benchmark_ref pi_2 369 3 $i london 3
done
for i in {1..4}; do
    # OMITTED: i=5..10
    run_benchmark_ref pi_2 1005 3 $i hiv 3
done

for i in {0..10}; do
    run_benchmark pi_1 61 1301 $i aarhus 3
    run_benchmark pi_1 369 1375 $i london 3
done
for i in {0..0}; do
    # OMITTED: i=1..10
    run_benchmark pi_1 1005 3693 $i hiv 3
done

for i in {1..10}; do
    run_benchmark_ref pi_1 61 5 $i aarhus 3
done
for i in {1..1}; do
    # OMITTED: i=2..10
    run_benchmark_ref pi_1 369 3 $i london 3
    run_benchmark_ref pi_1 1005 3 $i hiv 3
done


####################
### arabi, higgs ###
####################

for i in {0..10}; do
    run_benchmark pi_3 6980 43214 $i arabi 3
done
# OMITTED: higgs, fully
touch p0/output_pi_3_higgs.txt
touch p1/output_pi_3_higgs.txt
touch p2/output_pi_3_higgs.txt

for i in {0..10}; do
    run_benchmark pi_2 6980 43214 $i arabi 3
done
# OMITTED: higgs, fully
touch p0/output_pi_2_higgs.txt
touch p1/output_pi_2_higgs.txt
touch p2/output_pi_2_higgs.txt


#################################
### Scalability for MultiCent ###
#################################

# Using s = 11*n and 51*n and 101*n, i.e., #edges = 10* or 50* or 100* #nodes

# pi 3
for i in 0 1 2; do
for n in 100 200 300 400 500 600 700 800 900 1000 2000 3000 4000 5000 6000 7000 8000 9000 10000; do
    # OMITTED: 20k, 30k, 40k, 50k, 60k, 70k, 80k, 90k, 100k, 200k, 300k, 400k, 500k
    run_benchmark pi_3 $n $((11*n)) $i 11_d$i 3
done
for n in 100 200 300 400 500 600 700 800 900 1000 2000 3000 4000; do
    # OMITTED: 5k, 6k, 7k, 8k, 9k, 10k, 20k, 30k, 40k, 50k
    run_benchmark pi_3 $n $((51*n)) $i 51_d$i 3
done
for n in 100 200 300 400 500 600 700 800 900 1000 2000; do
    # OMITTED: 3k, 4k, 5k, 6k, 7k, 8k, 9k, 10k, 20k, 30k, 40k, 50k
    run_benchmark pi_3 $n $((101*n)) $i 101_d$i 3
done
done

# pi 2
for i in 0 1 2; do
for n in 100 200 300 400 500 600 700 800 900 1000 2000 3000 4000 5000 6000 7000 8000 9000 10000; do
    # OMITTED: 20k, 30k, 40k
    run_benchmark pi_2 $n $((11*n)) $i 11_d$i 3
done
for n in 100 200 300 400 500 600 700 800 900 1000 2000 3000; do
    # OMITTED: 4k, 5k, 6k, 7k, 8k, 9k, 10k, 20k, 30k, 40k
    run_benchmark pi_2 $n $((51*n)) $i 51_d$i 3
done
for n in 100 200 300 400 500 600 700 800 900 1000; do
    # OMITTED: 2k, 3k, 4k, 5k, 6k, 7k, 8k, 9k, 10k, 20k, 30k, 40k
    run_benchmark pi_2 $n $((101*n)) $i 101_d$i 3
done
done

# pi 1
for i in 0 1 2; do
for n in 100 200 300 400; do
    # OMITTED: 500, 600, 700, 800, 900
    run_benchmark pi_1 $n $((11*n)) $i 11_d$i 3
done
for n in 100 200; do
    # OMITTED: 300, 400, 500, 600, 700, 800, 900
    run_benchmark pi_1 $n $((51*n)) $i 51_d$i 3
done
for n in 100; do
    # OMITTED: 200, 300, 400, 500, 600, 700, 800, 900
    run_benchmark pi_1 $n $((101*n)) $i 101_d$i 3
done
done


#################################
### Scalability for Reference ###
#################################

# pi 3
for i in 1 2; do
for n in 100 200 300 400 500 600 700 800 900 1000; do
    # OMITTED: 2k, 3k, 4k, 5k, 6k, 7k, 8k
    run_benchmark_ref pi_3 $n 3 $i d$i 3
done
done

# pi 2
for i in 1 2; do
for n in 100 200 300 400 500 600 700 800 900 1000; do
    # OMITTED: 2k, 3k, 4k, 5k, 6k, 7k
    run_benchmark_ref pi_2 $n 3 $i d$i 3
done
done

# pi 1
for i in 1 2; do
for n in 100; do
    # OMITTED: 200, 300, 400, 500, 600
    run_benchmark_ref pi_1 $n 3 $i d$i 3
done
done

./network_off.sh || exit 1
