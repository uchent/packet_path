#!/bin/bash
NIC="ens192"
DEST_IP="192.168.1.200"
DEST_MAC="00:0c:29:8e:c0:07"
COUNT=10
CLONE_SKB=1000
PKT_SIZE=64

echo "rem_device_all" > /proc/net/pktgen/kpktgend_0
echo "add_device $NIC" > /proc/net/pktgen/kpktgend_0

echo "count $COUNT" > /proc/net/pktgen/$NIC
echo "clone_skb $CLONE_SKB" > /proc/net/pktgen/$NIC
echo "pkt_size $PKT_SIZE" > /proc/net/pktgen/$NIC
echo "dst $DEST_IP" > /proc/net/pktgen/$NIC
echo "dst_mac $DEST_MAC" > /proc/net/pktgen/$NIC
echo "delay 0" > /proc/net/pktgen/$NIC
echo "start" > /proc/net/pktgen/pgctrl

cat /proc/net/pktgen/$NIC