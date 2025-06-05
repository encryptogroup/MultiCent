#!/bin/sh
/sbin/tc qdisc del dev lo root 2> /dev/null || echo "Network emulation was turned off, nothing to deactivate"
