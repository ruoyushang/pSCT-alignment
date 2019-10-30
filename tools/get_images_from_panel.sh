#!/bin/bash

panel=$1

IP=$(mysql --password=$CTADBPASS -u CTAreadwrite --port=$CTADBPORT -h $CTADBHOST -D $CTAONLINEDB -s --execute="select mpcb_ip_address from Opt_MPMMapping where position=(${panel}) and end_date is NULL")

ssh root@$IP date +"%Y-%m-%d_%H-%M-%S"

mkdir -p ~/mpesimages/${panel}

scp root@$IP:~/mpesimages/edge_cam*_pic11_*.jpg  ~/mpesimages/${panel}/
