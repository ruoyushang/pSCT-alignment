#!/bin/bash

panel=$1

if [ -z "$2" ]
then
    wildcard=$(date +"%Y-%m-%d_%H-%M")
else
    wildcard=$2
fi

echo using wildcard $wildcard

IP=$(mysql --password=$CTADBPASS -u CTAreadwrite --port=$CTADBPORT -h $CTADBHOST -D $CTAONLINEDB -s --execute="select mpcb_ip_address from Opt_MPMMapping where position=(${panel}) and end_date is NULL")

echo Date on server: 
ssh root@$IP date +"%Y-%m-%d_%H-%M-%S"

mkdir -p ~/logs/passerver_logs/${panel}

scp root@$IP:/home/root/logs/passerver_logs ~/logs/passerver_logs/${panel}/passerver_logs_$wildcard
