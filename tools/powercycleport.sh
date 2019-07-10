#!/bin/sh

TYPE=$1
port=$2
sleeptime=30

if [ "$#" -ne 2 ]; then
    echo "Must provide two arguments"
    echo "./powercycleport.sh [mirror=0 or 1] [port number]"
    echo "Run like:"
    echo "./powercycleport.sh 0 1"
    exit
fi
echo "Powering down mirror $TYPE port $port ..."
if [ $TYPE -eq 0 ]; then # || [ "$TYPE" == "P" ]; then
        echo callps 0 PSoff $2
        callps 0 PSoff $2
elif [ $TYPE -eq 1 ]; then # || [ "$TYPE" == "M" ]; then
        echo callps 1 PSoff $2
        callps 1 PSoff $2
fi

sleep $sleeptime


echo "Powering up mirror $TYPE port $port ..."
if [ $TYPE -eq 0 ]; then  # || [ "$TYPE" == "P" ]; then
        echo callps 0 PSon $2
        callps 0 PSon $2
elif [ $TYPE -eq 1 ]; then  # || [ "$TYPE" == "M" ]; then
        echo callps 1 PSon $2
        callps 1 PSon $2
fi

sleep $sleeptime
echo "Done"
