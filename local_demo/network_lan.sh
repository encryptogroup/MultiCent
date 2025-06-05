#!/bin/sh
/sbin/tc qdisc del dev lo root 2> /dev/null || echo "Network emulation was turned off, activating now..."
/sbin/tc qdisc add dev lo root handle 1:0 htb default 10
/sbin/tc class add dev lo parent 1:0 classid 1:10 htb rate 1Gbit quantum 1500
/sbin/tc qdisc add dev lo parent 1:10 handle 10:0 netem delay 0.5ms 0.03ms 5%
