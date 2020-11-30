#!/bin/bash

ip_address=$1
panel_num=$2
screen_name=upgrade_etc

# only create a new screen session if one with current name doesn't exist yet
if ! screen -ls | grep -q $screen_name; then
    # -A for adaptive sizing
    # -dm for start in detached mode
    # -S to name the session
    screen -c helper/pas_serv_screen.config -AdmS $screen_name
fi

# open a new window in the screen session named $screen_name, and title this window by the panel's position;
screen -S $screen_name -X screen -t $panel_num
sleep 1
screen -S $screen_name -p $panel_num -X stuff $"scp -q helper/passerver.service root@$ip_address:/etc/systemd/system/\n"
sleep 1
screen -S $screen_name -p $panel_num -X stuff $"scp -q helper/passerver.env root@$ip_address:/etc/\n"
sleep 1
screen -S $screen_name -p $panel_num -X stuff $"scp -q helper/start_server.sh root@$ip_address:~/opcua/\n"
sleep 1
screen -S $screen_name -X screen -t $panel_num ssh root@$ip_address
sleep 3
screen -S $screen_name -p $panel_num -X stuff $"mkdir /etc/conf.d/\n"
screen -S $screen_name -p $panel_num -X stuff $"mv /etc/passerver.env /etc/conf.d/\n"
screen -S $screen_name -p $panel_num -X stuff $"sed -i 's/__IP_ADDRESS__/${ip_address}/g' /etc/conf.d/passerver.env \n"

screen -S $screen_name -p $panel_num -X stuff "echo DONE!\n"

