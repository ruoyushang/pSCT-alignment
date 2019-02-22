#!/bin/sh

mkdir -p /.ASF

ASFFILES=~/actuator/ASF_*.log
for ASF in $ASFFILES; do
    serial=$(echo $ASF | sed -e 's/.*ASF_\(.*\).log.*/\1/')
    if [ $serial -ne 0 ]; then
        echo -n " $serial"
        revolution=$(cat $ASF | awk '{print $3}')
        angle=$(cat $ASF | awk '{print $4}')
        echo "2000 01 01 00 00 00 $revolution $angle 0 0 0 0 0 0 0 0 0 0 0 0 0 0" > /.ASF/.ASF_$serial.log
    fi
done

exit 0;
