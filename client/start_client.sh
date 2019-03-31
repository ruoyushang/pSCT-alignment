#!/usr/bin/env bash
usage() { echo "Usage: $0  [-a (all)] [panel number(s)] [-h]" 1>&2; exit 1; }

all=false

while getopts "ah" o; do
    case "${o}" in
    a) all=true ;;
    h) usage ;;
    esac
done

shift $((OPTIND-1))

if $all ; then

printf "Reading from database...\n"

    PANELS=()
    if [[ -z $MYSQL_USER || -z $MYSQL_PASSWORD || -z $MYSQL_DATABASE || -z $MYSQL_HOST || -z $MYSQL_PORT ]]; then
        echo "Database credentials not set - check $MYSQL_USER, $MYSQL_PASSWORD, $MYSQL_DATABASE, $MYSQL_HOST, $MYSQL_PORT"
        exit 1
    fi
    while read -r position;
    do
         if [[ $position = "0" ]] || [[ $position = "1" ]] || [[ ${position:1:1} = "0" ]];
         then
             continue
         fi
         PANELS+=($position)
    done < <(mysql --user=${MYSQL_USER} --password=${MYSQL_PASSWORD} --database=${MYSQL_DATABASE} --host=${MYSQL_HOST} --port=${MYSQL_PORT}-ss -e "SELECT position FROM Opt_MPMMapping")
else
    PANELS=("$@")
fi

count=$((${#PANELS[@]}))

printf "Starting client...\n"

opcua-client &>> /dev/null &

../sdk/bin/p2pasclient "${PANELS[@]}"
