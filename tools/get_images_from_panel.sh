#!/bin/bash

panel=$1

if [ -z "$2" ]
then
    wildcard=$(date +"%Y-%m-%d")
else
    wildcard=$2
fi

echo using wildcard $wildcard

IP=$(mysql --password=$CTADBPASS -u CTAreadwrite --port=$CTADBPORT -h $CTADBHOST -D $CTAONLINEDB -s --execute="select mpcb_ip_address from Opt_MPMMapping where position=(${panel}) and end_date is NULL")

echo Date on server: 
ssh root@$IP date +"%Y-%m-%d_%H-%M-%S"

mkdir -p ~/mpesimages/${panel}

scp root@$IP:~/mpesimages/edge_cam*_pic11_*$wildcard*.jpg  ~/mpesimages/${panel}/
