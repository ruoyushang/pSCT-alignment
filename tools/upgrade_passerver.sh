#!/bin/bash

ip_address=$1
panel_num=$2
screen_name=upgrade

# only create a new screen session if one with current name doesn't exist yet
if ! screen -ls | grep -q $screen_name; then
    # -A for adaptive sizing
    # -dm for start in detached mode
    # -S to name the session
    screen -c helper/pas_serv_screen.config -AdmS $screen_name
fi

# open a new window in the screen session named $screen_name, and title this window by the panel's position;
# ssh into the panel right away
screen -S $screen_name -X screen -t $panel_num

screen -S $screen_name -p $panel_num -X stuff $"scp -q ~/filesToCopy/passerver root@$ip_address:~/opcua/sdk/bin/\n"
screen -S $screen_name -p $panel_num -X stuff "scp -q ~/home/user/repos/bryan/pSCT-alignment/server/start_server.sh root@$ip_address:~/opcua/\n"
screen -S $screen_name -p $panel_num -X stuff "echo DONE!\n"

