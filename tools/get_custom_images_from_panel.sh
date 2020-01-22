#!/bin/bash

if [ -z $1 ]
then
    echo "Usage: $0 panelID [lookback_n_hour]"
fi

panel=$1
subdir=20191110

IP=$(mysql --password=$CTADBPASS -u CTAreadwrite --port=$CTADBPORT -h $CTADBHOST -D $CTAONLINEDB -s --execute="select mpcb_ip_address from Opt_MPMMapping where position=(${panel}) and end_date is NULL")
server_date=$(ssh root@$IP date +"%Y-%m-%d_%H_%M_%S")
echo Date on server: $server_date

# below is a offset in hour backwards in case you took a break after MPES was read lol
if [ -z "$2" ]
then
    houroffset=0
else
    houroffset=$2 
fi 

echo "Looking for images taken $houroffset hr ago..."
server_hour_minus=$(( ${server_date:11:2} - houroffset ))
server_hour_minus=$(printf "%.2d" $server_hour_minus)
if [[ $server_hour_minus -lt 0 ]]; 
then 
    server_hour_minus=$(( 24 + $server_hour_minus )); 
    server_day=$(printf "%.2d" $(( ${server_date:8:2} - 1 )))
    #if [[ $server_day -lt 0 ]];
    if [[ $(( ${server_date:8:2} - 1 )) -lt 0 ]]; then echo "you are doing this from one month to another, sorry I'm too lazy to count number of days of every month here, bye"; exit 1; fi
    server_hour_minus="${server_date:0:8}${server_day}_$server_hour_minus"
else
    server_hour_minus=${server_date:0:11}$server_hour_minus
fi
echo "i.e., server time hour mark $server_hour_minus"

wildcard=$server_hour_minus

echo using wildcard $wildcard



mkdir -p ~/mpesimages/$subdir/${panel}

scp root@$IP:~/mpesimages/edge_cam*_pic11_*$wildcard*.jpg  ~/mpesimages/$subdir/${panel}/
