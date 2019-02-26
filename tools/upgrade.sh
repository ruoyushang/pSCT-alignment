#!/bin/bash
#usage() {
#    echo -e "Usage: "
#    echo ".${0##*/} -f <ip_to_connect> [OPTIONS]"
#
#}
#
#while getopts ":c:p:f:t:" opt; do
#    case $opt in
#        c )
#            echo "channel"
#            ;;
#        p )
#            echo "panel_number"
#            ;;
#        f )
#            echo "current_ip"
#            ;;
#        t )
#            echo "new_ip"
#            ;;
#        : )
#            echo "Invalid Option: -$OPTARG requires and argument" 1>&2
#            exit 1
#            ;;
#
#        \? )
#            usage
#            ;;
#    esac
#done
#
#[ -z $current_ip ] && usage

old_ip_address=$1
new_ip_address=$2
panelnum=$3
pedb_channel=$4


#if [[ -n $new_ip_address ]]; then # need to update the ip address as well
#    ip_address=$old_ip_address
#    echo -n "Updating the ip address of $old_ip_address to $new_ip_address..."
#    scp -q helper/interfaces_template root@$ip_address:/etc/network/interfaces
#    ssh -n -q root@$ip_address "sed -i \"s/NEW_IP_ADDRESS/$new_ip_address/\" /etc/network/interfaces"
#    echo " DONE!"
#
#    if [[ -n $panelnum ]]; then
#        echo "Updating the database entry for panel $panelnum:"
#
#        dbcmd="update Opt_MPMMapping set mpcb_ip_address=\"$new_ip_address\""
#        [[ -n $pedb_channel ]] && dbcmd=$dbcmd", pedb_channel=$pedb_channel"
#        dbcmd=$dbcmd" where position=$panelnum";
#
#        echo $dbcmd
#        mysql --password=$CTADBPASS -u CTAreadwrite --port=$CTADBPORT -h $CTADBHOST -D $CTAONLINEDB --execute="$dbcmd"
#        dbStatus=$?
#        if (( dbStatus != 0 )); then
#            echo "Database update wasn't successful. Check your command and rerun!"
#            exit $dbStatus
#        fi
#        echo "SUCESS!"
#        echo
#    else
#        echo
#        echo "You have provided a pair of IP addresses but no panel number. Can't update the database records!"
#        echo "You must provide a panel number to update the database records!"
#    fi
#fi

#echo -n "Updating internet time acquisition scripts..."
#scp -q helper/netstart.sh root@$ip_address:/etc/init.d/
#scp -q helper/ntp.conf root@$ip_address:/etc/
#echo " DONE!"
#
#echo -n "Upgrading the ASF file structure... Actutor"
#scp -q helper/localactuatorupgradescript.sh root@$ip_address:
#ssh -n -q root@$ip_address "sh localactuatorupgradescript.sh"
#ssh -n -q root@$ip_address "rm localactuatorupgradescript.sh"
#echo " DONE!"

#if [[ -n $panelnum ]]; then
#    echo -n "Updating hostname to \"opt_$panelnum\"... "
#    ssh -n -q root@$ip_address "echo opt_$panelnum > /etc/hostname"
#    echo " DONE!"
#fi
#
#echo -n "Creating the ~/mpesimages folder"
#ssh -n -q root@$ip_address "mkdir -p mpesimages"
#echo " DONE!"
#
ip_address=$old_ip_address
echo -n "Upgrading the mpesserver binary..."
scp -q helper/mpesserver root@$ip_address:~/opcua/sdk/bin/
#scp -q helper/start_server root@$ip_address:~/opcua/
echo " DONE!"
#
#echo -n "Updating the panelcontrol binary..."
#ssh -n -q root@$ip_address "cd ~/actuatornew; sed -i \"s/romulus.ucsc.edu/10.0.50.114/\" panelcontrol.cpp"
#echo " ALL DONE!"
#
#
#if [[ -n $old_ip_address ]]; then # need to restart
#    echo
#    echo "Restarting $ip_address now..."
#    ssh -n -q root@$ip_address "shutdown -r now"
#fi
#
#echo

exit 0
