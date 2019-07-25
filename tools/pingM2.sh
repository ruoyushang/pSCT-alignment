#!/bin/bash

while read -r line || [[ -n $line ]]; do
    panel=`echo $line | cut -d ' ' -f1`
    newip=`echo $line | cut -d ' ' -f3`
    echo -e "\tPinging panel $panel..."
    #echo "./forceipccd.sh $SERIAL $newip &"
    ping -c 1 $newip  > /dev/null && echo "up" || echo "*** Panel $panel is down! ***"
done < mpcb_map_M2.dat
