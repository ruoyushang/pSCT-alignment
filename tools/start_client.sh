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
         if [[ ${position} = "0" ]] || [[ ${position} = "1" ]] || [[ ${position:1:1} = "0" ]];
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
screen_name=client

if ! screen -ls | grep -q $screen_name; then
    # -A for adaptive sizing
    # -dm for start in detached mode
    # -S to name the session
    screen -c helper/pas_serv_screen.config -AdmS $screen_name
fi

# start the commands we need in the newly created/named screen window; \n to simulate keypress
screen -S $screen_name -X stuff $"../sdk/bin/p2pasclient ${PANELS[*]} | tee ${HOME}/logs/p2pasclient_log_`date +%Y%m%d%H%M%S`.txt\n"


