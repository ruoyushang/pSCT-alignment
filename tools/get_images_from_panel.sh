#!/bin/bash

panel=$1

IP=$(mysql --password=$CTADBPASS -u CTAreadwrite --port=$CTADBPORT -h $CTADBHOST -D $CTAONLINEDB -s --execute="select mpcb_ip_address from Opt_MPMMapping where position=(${panel})")


ssh root@$IP date +"%Y-%m-%d_%H-%M-%S"

mkdir -p ~/p2pas/mpesimages/${panel}

rsync -uvp root@$IP:~/mpesimages/edge_cam*_pic19_*.jpg  ~/p2pas/mpesimages/${panel}/
