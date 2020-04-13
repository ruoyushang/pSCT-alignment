#!/bin/bash

subdir=20191215

if [ -z $1 ]
then
    echo "Usage: $0 panelID [optional lookback_n_hour or wildcard]"
    echo "Can run in 3 modes like the example below: "
    echo "./get_custom_images_from_panel.sh 1413 will grab all images taken within the current hour; "
    echo "./get_custom_images_from_panel.sh 1413 12 will grab all images taken 12 hours ago, only when a integer is provided as the optional second arg (this part has caveats, so when there is a problem, use wildcard instead, see below); "
    echo "./get_custom_images_from_panel.sh 1413 2019-11-11_01_02 will grab all images with file name matching the wildcard string 2019-11-11_01_02; "
    echo "Will save images to ~/mpesimages/$subdir/panel_id; if you don't like it, change the subdir in the script. "
    exit 0
fi

panel=$1
echo "Will save images to ~/mpesimages/$subdir/${panel}; if you don't like it, change the subdir in the script. "

IP=$(mysql --password=$CTADBPASS -u CTAreadwrite --port=$CTADBPORT -h $CTADBHOST -D $CTAONLINEDB -s --execute="select mpcb_ip_address from Opt_MPMMapping where position=(${panel}) and end_date is NULL")
server_date=$(ssh root@$IP date +"%Y-%m-%d_%H_%M_%S")
echo Date on server: $server_date

if ! [ -z "$2" ] && ! [[ "$2" =~ ^[0-9]+$ ]] 
then
    echo "You seem to have provided a string, using it as a wild card"
    wildcard=$2
    echo $wildcard
    #exit 0
else
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
fi

echo Using wildcard $wildcard



echo Saving images to ~/mpesimages/$subdir/${panel}

mkdir -p ~/mpesimages/$subdir/${panel}

scp root@$IP:~/mpesimages/edge_cam*_pic11_*$wildcard*.jpg  ~/mpesimages/$subdir/${panel}/
