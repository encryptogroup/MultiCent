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

# scalability.sh

###
# Ours with s = 101*n and 51*n and 11*n
###

# pi 3
for i in 0 1 2; do
for n in 100 200 300 400 500 600 700 800 900; do
    ./pi_3_benchmark --pid $1 -n $n -s $((101*n)) -d $i -o output_pi_3_101_d$i.txt -r 3 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
    ./pi_3_benchmark --pid $1 -n $n -s $((51*n)) -d $i -o output_pi_3_51_d$i.txt -r 3 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
    ./pi_3_benchmark --pid $1 -n $n -s $((11*n)) -d $i -o output_pi_3_11_d$i.txt -r 3 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
done
for n in 1000 2000 3000 4000 5000 6000 7000 8000 9000; do
    ./pi_3_benchmark --pid $1 -n $n -s $((101*n)) -d $i -o output_pi_3_101_d$i.txt -r 1 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
    ./pi_3_benchmark --pid $1 -n $n -s $((51*n)) -d $i -o output_pi_3_51_d$i.txt -r 1 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
    ./pi_3_benchmark --pid $1 -n $n -s $((11*n)) -d $i -o output_pi_3_11_d$i.txt -r 1 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
done
for n in 10000 20000 30000 40000 50000; do
    ./pi_3_benchmark --pid $1 -n $n -s $((101*n)) -d $i -o output_pi_3_101_d$i.txt -r 1 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
    ./pi_3_benchmark --pid $1 -n $n -s $((51*n)) -d $i -o output_pi_3_51_d$i.txt -r 1 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
    ./pi_3_benchmark --pid $1 -n $n -s $((11*n)) -d $i -o output_pi_3_11_d$i.txt -r 1 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
done
done

###
# Reference
###

# pi 3
for i in 1 2; do
for n in 100 200 300 400 500 600 700 800 900; do
    ./pi_3_ref_benchmark --pid $1 -n $n -l 3 -d $i -o output_pi_3_ref_d$i.txt -r 3 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
done
for n in 1000 2000 3000 4000 5000 6000 7000 8000; do
    ./pi_3_ref_benchmark --pid $1 -n $n -l 3 -d $i -o output_pi_3_ref_d$i.txt -r 1 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
done
done
