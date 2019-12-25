#!/bin/bash

ETH=$1
RESULT_PCAP=$2
SEND_PCAP=$3
DUMP_ETH=$4

if [ -z "$5" ]
then
    sleep_time=1
else
    sleep_time=$5
fi
echo $ETH $RESULT_PCAP $ETH $SEND_PCAP $sleep_time
nohup tcpdump -i $DUMP_ETH -w $RESULT_PCAP &
tcpdumppid=$!
tcpreplay -M 800 -i $ETH $SEND_PCAP
sleep $sleep_time
kill -15 $tcpdumppid

