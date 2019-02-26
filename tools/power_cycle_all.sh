#!/bin/sh

NPORTS=8
MIRROR=1
TYPE=$1

if [ $TYPE -eq 0 ] || [ $TYPE -eq 2 ]; then
for ((i=0;i<=$NPORTS;i++)); do
        callps ${MIRROR} PSoff $i
    done
fi

if [ $TYPE -eq 2 ]; then 
    sleep 50
fi

if [ $TYPE -eq 1 ] || [ $TYPE -eq 2 ]; then
for ((i=0;i<=$NPORTS;i++)); do
        callps ${MIRROR} PSon $i
    done
fi
#sleep 50
#echo "Power up all"
