#!/bin/bash

while read -r ip || [[ -n $ip ]]; do
    echo $ip
    #scp  "ntp.conf" "root@$ip:/etc/" 
    #ssh "root@$ip" 'systemctl stop ntpd' < /dev/null 
    #ssh "root@$ip" 'ntpdate 172.17.0.1' < /dev/null
    #ssh "root@$ip" 'systemctl start ntpd' < /dev/null
    ssh "root@$ip" 'systemctl restart ntpd' < /dev/null
done < all_panel_ips.txt
