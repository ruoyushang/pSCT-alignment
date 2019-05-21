#! /usr/bin/env bash

panel=$1

CHANNEL=$(mysql --password=$CTADBPASS -u CTAreadwrite --port=$CTADBPORT -h $CTADBHOST -D $CTAONLINEDB -s --execute="select pedb_channel from Opt_MPMMapping where position=(${panel})")

if [ $panel -gt 2000 ]; then  
    MIRROR=1 
elif [ $panel -lt 2000 ]; then 
    MIRROR=0 
fi

INTERVAL_TIME_SEC=5
while true; do
    sleep $INTERVAL_TIME_SEC
    result=`callps $MIRROR GetCurrent $CHANNEL`
    today=`date '+%Y-%m-%d %H:%M:%S'`
    echo -e "$today\t$panel\t$result" >> ~/logs/currents/${panel}_currents.log
done


