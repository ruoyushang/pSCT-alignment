#!/bin/bash

PORT=$1

if [ ! -z $2 ] 
then 
    # $2 # of steps was given
    nsteps=$2
else
    # $2 was not given
    nsteps=800
fi

echo "We are testing the actuator on port $PORT"
read -r -p "Are you sure? [y/n] " response
if [[ "$response" =~ ^([yY][eE][sS]|[yY])+$ ]]
then
    echo "Good..."
else
    echo "Exit"
    exit 0
fi

./cbc status
./cbc measure 1 0 100 0 100 5

read -p "Press enter to continue enable all"
./cbc enable all
echo "Enabled all"
./cbc status

read -r -p "Now we plan to extend $nsteps steps and retract $nsteps steps. Are you sure? [y/n] " response
if [[ "$response" =~ ^([yY][eE][sS]|[yY])+$ ]]
then
    echo "========"
    echo "====Important encoder voltage readings below===="
    echo "========"
    echo "Measurement before extending"
    ./cbc measure 1 $PORT 100 0 100 5
    echo "========"
    echo "Extending..."
    ./cbc step $PORT $nsteps
    echo "========"
    echo "Measurement after extending"
    ./cbc measure 1 $PORT 100 0 100 5
    echo "========"
    echo "Retracting..."
    ./cbc step $PORT -$nsteps
    echo "========"
    echo "Measurement after retracting"
    ./cbc measure 1 $PORT 100 0 100 5
    echo "========"
    echo "========"
else
    echo "Seems you are not sure. Disabling all..."
    ./cbc disable_all 
    echo "Exit"
    exit 0
fi


read -r -p "Now do you want to do the same with high current mode?? (Will extend $nsteps steps and retract $nsteps steps in high current mode.) Are you sure? [y/n] " response
if [[ "$response" =~ ^([yY][eE][sS]|[yY])+$ ]]
then
    echo "========"
    echo "Enabling high current mode"
    ./cbc enable_hi_current
    ./cbc status
    echo "========"
    echo "====Important encoder voltage readings below===="
    echo "========"
    echo "Measurement before extending"
    ./cbc measure 1 $PORT 100 0 100 5
    echo "========"
    echo "Extending..."
    ./cbc step $PORT $nsteps
    echo "========"
    echo "Measurement after extending"
    ./cbc measure 1 $PORT 100 0 100 5
    echo "========"
    echo "Retracting..."
    ./cbc step $PORT -$nsteps
    echo "========"
    echo "Measurement after retracting"
    ./cbc measure 1 $PORT 100 0 100 5
    echo "========"
    echo "Disabling high current mode"
    ./cbc disable_hi_current
    ./cbc status
    echo "========"
else
    echo "Seems you are not sure. Disabling all..."
    ./cbc disable_all 
    ./cbc status
    echo "Exit"
    exit 0
fi

read -p "Press enter to disable all"
./cbc disable_all 
./cbc status

echo "Seems we tested it. Done"


