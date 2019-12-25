#!/bin/bash


tcpreplay -M 10 -i $1 $2 >> /dev/null
echo $?