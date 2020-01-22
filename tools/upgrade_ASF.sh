#!/bin/bash

pos=$1
screen_name=upgrade

ip_address=$(mysql -h $CTADBHOST -P $CTADBPORT -u CTAreadonly --password=readCTAdb -D $CTAONLINEDB --execute="select mpcb_ip_address from Opt_MPMMapping where position=${pos}" | xargs echo -n | cut -d ' ' -f2)

# only create a new screen session if one with current name doesn't exist yet
if ! screen -ls | grep -q $screen_name; then
    # -A for adaptive sizing
    # -dm for start in detached mode
    # -S to name the session
    screen -c helper/pas_serv_screen.config -AdmS $screen_name
fi

# open a new window in the screen session named $screen_name, and title this window by the panel's position;
# ssh into the panel right away
screen -S $screen_name -X screen -t $pos ssh root@$ip_address
sleep 1.5
# start the commands we need in the newly created/named screen window; \n to simulate keypress
screen -S $screen_name -p $pos -X stuff $"cp -a ~/.ASF/. /.ASF/\n"
