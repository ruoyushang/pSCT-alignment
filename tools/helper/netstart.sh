#!/bin/sh
/etc/init.d/cbcbinary/cbc initialize
sleep 1
/etc/init.d/cbcbinary/cbc enableUSB_all
sleep 10
/etc/init.d/networking start
sleep 5

ntpdate -u 10.0.0.142
sleep 5
hwclock -w

exit 0
# $ update-rc.d netstart.sh start 80 3 4 5 .
