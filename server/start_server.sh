#!/bin/bash

# General-purpose script to start a single server on a panel
# with logging of output to a text file.
# NOTE: Requires the existence of a ServerConfig_template.xml file in the executable directory.
# NOTE: Defaults to an executable directory of /home/root/pSCT-alignment/sdk/bin if not provided

#if [[ "$#" -ne 1 ]]; then
#    echo "Usage: `basename $0` <PANEL NUMBER> <LOG LEVEL (optional)> <SDK DIRECTORY PATH (optional)>"
#    exit 1;
#fi

if [[ -n ${LOCALIP} ]]; then
    IP=${LOCALIP}
else
    IP="127.0.0.1"
fi

# Create log directory if it doesn't exist
[[ -d $HOME/logs ]] || mkdir $HOME/logs

PANEL_NUM=$1
LOGLEVEL=${2-"info"}
EXECDIR=${3-"/home/root/opcua/sdk/bin"}

# prepare config file -- set the IP in the template
configtmplt=${EXECDIR}"/ServerConfig_template.xml"
configfile=${configtmplt/_template/}
cp ${configtmplt} ${configfile}
sed -i -e "s/__IP_ADDRESS__/${IP}/" ${configfile}

${EXECDIR}/passerver ${PANEL_NUM} -l ${LOGLEVEL}
