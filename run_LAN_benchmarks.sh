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

# small_datasets.sh

for i in {0..10}; do
    # Aarhus
    ./pi_3_benchmark --pid $1 -n 61 -s 1301 -d $i -o output_pi_3_aarhus.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
    # London
    ./pi_3_benchmark --pid $1 -n 369 -s 1375 -d $i -o output_pi_3_london.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
    # hiv
    ./pi_3_benchmark --pid $1 -n 1005 -s 3693 -d $i -o output_pi_3_hiv.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
done

for i in {1..10}; do
    # Aarhus
    ./pi_3_ref_benchmark --pid $1 -n 61 -l 5 -d $i -o output_pi_3_ref_aarhus.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
    # London
    ./pi_3_ref_benchmark --pid $1 -n 369 -l 3 -d $i -o output_pi_3_ref_london.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
    # hiv
    ./pi_3_ref_benchmark --pid $1 -n 1005 -l 3 -d $i -o output_pi_3_ref_hiv.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
done

for i in {0..10}; do
    # Aarhus
    ./pi_2_benchmark --pid $1 -n 61 -s 1301 -d $i -o output_pi_2_aarhus.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
    # London
    ./pi_2_benchmark --pid $1 -n 369 -s 1375 -d $i -o output_pi_2_london.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
    # hiv
    ./pi_2_benchmark --pid $1 -n 1005 -s 3693 -d $i -o output_pi_2_hiv.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
done

for i in {1..10}; do
    # Aarhus
    ./pi_2_ref_benchmark --pid $1 -n 61 -l 5 -d $i -o output_pi_2_ref_aarhus.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
    # London
    ./pi_2_ref_benchmark --pid $1 -n 369 -l 3 -d $i -o output_pi_2_ref_london.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
    # hiv
    ./pi_2_ref_benchmark --pid $1 -n 1005 -l 3 -d $i -o output_pi_2_ref_hiv.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
done

for i in {0..10}; do
    # Aarhus
    ./pi_1_benchmark --pid $1 -n 61 -s 1301 -d $i -o output_pi_1_aarhus.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
    # London
    ./pi_1_benchmark --pid $1 -n 369 -s 1375 -d $i -o output_pi_1_london.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
    # hiv
    ./pi_1_benchmark --pid $1 -n 1005 -s 3693 -d $i -o output_pi_1_hiv.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
done

for i in {1..10}; do
    # Aarhus
    ./pi_1_ref_benchmark --pid $1 -n 61 -l 5 -d $i -o output_pi_1_ref_aarhus.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
done

for i in {1..6}; do
    # London
    ./pi_1_ref_benchmark --pid $1 -n 369 -l 3 -d $i -o output_pi_1_ref_london.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
done

# hiv../build
./pi_1_ref_benchmark --pid $1 -n 1005 -l 3 -d 1 -o output_pi_1_ref_hiv.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
# sleep 5
rotate_ports



# bigger_datasets.sh

for i in {0..10}; do
    # arabi
    ./pi_3_benchmark --pid $1 -n 6980 -s 43214 -d $i -o output_pi_3_arabi.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
    # higgs
    ./pi_3_benchmark --pid $1 -n 304691 -s 1415653 -d $i -o output_pi_3_higgs.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
done

for i in {0..10}; do
    # arabi 
    ./pi_2_benchmark --pid $1 -n 6980 -s 43214 -d $i -o output_pi_2_arabi.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
    # higgs
    ./pi_2_benchmark --pid $1 -n 304691 -s 1415653 -d $i -o output_pi_2_higgs.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
done



# scalability.sh

###
# Ours with s = 101*n and 51*n and 11*n
###

# pi 3
for i in 0 1 2; do
for n in 100 200 300 400 500 600 700 800 900; do
    ./pi_3_benchmark --pid $1 -n $n -s $((101*n)) -d $i -o output_pi_3_101_d$i.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
    ./pi_3_benchmark --pid $1 -n $n -s $((51*n)) -d $i -o output_pi_3_51_d$i.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
    ./pi_3_benchmark --pid $1 -n $n -s $((11*n)) -d $i -o output_pi_3_11_d$i.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
done
for n in 1000 2000 3000 4000 5000 6000 7000 8000 9000; do
    ./pi_3_benchmark --pid $1 -n $n -s $((101*n)) -d $i -o output_pi_3_101_d$i.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
    ./pi_3_benchmark --pid $1 -n $n -s $((51*n)) -d $i -o output_pi_3_51_d$i.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
    ./pi_3_benchmark --pid $1 -n $n -s $((11*n)) -d $i -o output_pi_3_11_d$i.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
done
for n in 10000 20000 30000 40000 50000; do
    ./pi_3_benchmark --pid $1 -n $n -s $((101*n)) -d $i -o output_pi_3_101_d$i.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
    ./pi_3_benchmark --pid $1 -n $n -s $((51*n)) -d $i -o output_pi_3_51_d$i.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
    ./pi_3_benchmark --pid $1 -n $n -s $((11*n)) -d $i -o output_pi_3_11_d$i.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
done
done

# pi 2
for i in 0 1 2; do
for n in 100 200 300 400 500 600 700 800 900; do
    ./pi_2_benchmark --pid $1 -n $n -s $((101*n)) -d $i -o output_pi_2_101_d$i.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
    ./pi_2_benchmark --pid $1 -n $n -s $((51*n)) -d $i -o output_pi_2_51_d$i.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
    ./pi_2_benchmark --pid $1 -n $n -s $((11*n)) -d $i -o output_pi_2_11_d$i.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
done
for n in 1000 2000 3000 4000 5000 6000 7000 8000 9000; do
    ./pi_2_benchmark --pid $1 -n $n -s $((101*n)) -d $i -o output_pi_2_101_d$i.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
    ./pi_2_benchmark --pid $1 -n $n -s $((51*n)) -d $i -o output_pi_2_51_d$i.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
    ./pi_2_benchmark --pid $1 -n $n -s $((11*n)) -d $i -o output_pi_2_11_d$i.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
done
for n in 10000 20000 30000 40000; do
    ./pi_2_benchmark --pid $1 -n $n -s $((101*n)) -d $i -o output_pi_2_101_d$i.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
    ./pi_2_benchmark --pid $1 -n $n -s $((51*n)) -d $i -o output_pi_2_51_d$i.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
    ./pi_2_benchmark --pid $1 -n $n -s $((11*n)) -d $i -o output_pi_2_11_d$i.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
done
done

# pi 1
for i in 0 1 2; do
for n in 100 200 300 400 500; do
    ./pi_1_benchmark --pid $1 -n $n -s $((101*n)) -d $i -o output_pi_1_101_d$i.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
    ./pi_1_benchmark --pid $1 -n $n -s $((51*n)) -d $i -o output_pi_1_51_d$i.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
    ./pi_1_benchmark --pid $1 -n $n -s $((11*n)) -d $i -o output_pi_1_11_d$i.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
done
for n in 600 700 800 900; do
    ./pi_1_benchmark --pid $1 -n $n -s $((101*n)) -d $i -o output_pi_1_101_d$i.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
    ./pi_1_benchmark --pid $1 -n $n -s $((51*n)) -d $i -o output_pi_1_51_d$i.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
    ./pi_1_benchmark --pid $1 -n $n -s $((11*n)) -d $i -o output_pi_1_11_d$i.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
done
done

# pi 3 only with s=11*n and higher
for i in 0 1 2; do
for n in 60000 70000 80000 90000 100000 200000 300000 400000 500000; do
    ./pi_3_benchmark --pid $1 -n $n -s $((11*n)) -d $i -o output_pi_3_11_d$i.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
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
    ./pi_3_ref_benchmark --pid $1 -n $n -l 3 -d $i -o output_pi_3_ref_d$i.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
done
for n in 1000 2000 3000 4000 5000 6000 7000 8000; do
    ./pi_3_ref_benchmark --pid $1 -n $n -l 3 -d $i -o output_pi_3_ref_d$i.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
done
done

# pi 2
for i in 1 2; do
for n in 100 200 300 400 500 600 700 800 900; do
    ./pi_2_ref_benchmark --pid $1 -n $n -l 3 -d $i -o output_pi_2_ref_d$i.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
done
for n in 1000 2000 3000 4000 5000 6000 7000; do
    ./pi_2_ref_benchmark --pid $1 -n $n -l 3 -d $i -o output_pi_2_ref_d$i.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
done
done

# pi 1
for i in 1 2; do
for n in 100 200 300; do
    ./pi_1_ref_benchmark --pid $1 -n $n -l 3 -d $i -o output_pi_1_ref_d$i.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
done
for n in 400 500 600; do
    ./pi_1_ref_benchmark --pid $1 -n $n -l 3 -d $i -o output_pi_1_ref_d$i.txt -r 10 --net-config netconfig.json --port $CURRENT_PORT
    # sleep 5
    rotate_ports
done
done
