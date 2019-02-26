#!/bin/bash

while read -r line || [[ -n $line ]]; do
    SERIAL=`echo $line | cut -d ' ' -f4`
    curip=`echo $line | cut -d ' ' -f1`
    newip=`echo $line | cut -d ' ' -f2`
    echo -e "\tUpgrading panel $pos @ $curip to $newip"
    ./upgradeccd.sh $SERIAL $newip &
done < ccd_map.dat
