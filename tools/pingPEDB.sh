#!/bin/bash

echo -e "\tPinging PEDB1..."
ping -c 1 172.17.1.196  > /dev/null && echo "up" || echo "*** PEDB 1 is down! ***"
echo -e "\tPinging PEDB2..."
ping -c 1 172.17.1.197  > /dev/null && echo "up" || echo "*** PEDB 2 is down! ***"
