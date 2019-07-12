#!/bin/bash

while read -r line || [[ -n $line ]]; do
    SERIAL=`echo $line | cut -d ' ' -f4`
    echo "========"
    echo "Looking for camera with serial $SERIAL"
    camera-ip-conf-cli -i -s $SERIAL
done < ccd_map.dat
