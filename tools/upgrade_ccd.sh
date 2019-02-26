#!/bin/bash
SERIAL=$1
NEWIP=$2

SUBNET="255.255.0.0"
GATEWAY="172.17.0.1"

camera-ip-conf-cli set dhcp=on static=off ip=$NEWIP subnet=$SUBNET gateway=$GATEWAY -s $SERIAL 

