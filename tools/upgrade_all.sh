#!/bin/bash

while read -r line || [[ -n $line ]]; do
    pos=`echo $line | cut -d ' ' -f1`
    curip=`echo $line | cut -d ' ' -f3`
    newip=`echo $line | cut -d ' ' -f3`
    echo -e "\tUpgrading panel $pos @ $newip"
    ./upgrade_passerver.sh $newip
done < mpcb_map.dat
