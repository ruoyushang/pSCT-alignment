#!/bin/bash

today=`date '+%Y-%m-%d__%H_%M_%S'`
logfile=$HOME/logs/passerver_operations_${today}.log

if [[ "$#" -ne 1 ]]; then
    echo "Usage: `basename $0` <PANEL NUMBER>"
    exit 1;
fi

if [[ -n ${LOCALIP} ]]; then
    IP=${LOCALIP}
else
    IP="127.0.0.1"
fi


PANEL_NUM=$1
SDKDIR="/home/root/pSCT-alignment/sdk"

# prepare config file -- set the IP in the template
configtmplt=${SDKDIR}"/bin/ServerConfig_template.xml"
configfile=${configtmplt/_template/}
cp ${configtmplt} ${configfile}

sed -i -e "s/__IP_ADDRESS__/${IP}/" ${configfile}

# this clears the log from the previous time
echo `date` > ${logfile}
${SDKDIR}/bin/passerver ${PANEL_NUM} |& tee -a ${logfile}
