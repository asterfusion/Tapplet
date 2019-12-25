#!/bin/bash

DUMP_ETH=$1
RESULT_PCAP=$2
COUNT=$3

tcpdump -i $DUMP_ETH -c $COUNT -w $RESULT_PCAP &
