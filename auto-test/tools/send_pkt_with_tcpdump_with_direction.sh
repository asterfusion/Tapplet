#!/bin/bash

ETH=$1
RESULT_PCAP=$2
SEND_PCAP=$3
DUMP_ETH=$4
DUMP_DIRECATION=$5

if [ -z "$6" ]
then
    sleep_time=1
else
    sleep_time=$6
fi
echo $ETH $RESULT_PCAP $ETH $SEND_PCAP $sleep_time
nohup tcpdump -i $DUMP_ETH -Q $DUMP_DIRECATION -w $RESULT_PCAP &
tcpdumppid=$!
tcpreplay -p 1000 -i $ETH $SEND_PCAP
sleep $sleep_time
kill -15 $tcpdumppid

