#!/usr/bin/env bash
set -u
# Script to start simulation mode servers.
# Receives either a list of panel (position) numbers, or the option -a for all
# when "all" is selected, it will read all panels from the database except
# the test panels 0, 1, and any optical table panels (1001, 2001)

function interrupt() {
  printf "\nKilling all servers and exiting...\n"
  killall -9 -q "passerver"
  printf "Done.\n"
}
usage() { echo "Usage: $0  [-a (all)] [-h (help)] [-b (background)] [panel number(s)]" 1>&2; exit 1; }

all=false
background=false
keep_config=false
extension=".ini"

while getopts "ahbk" o; do
    case "${o}" in
    a) all=true ;;
    b) background=true ;;
    k) keep_config=true ;;
    h) usage ;;
    esac
done

shift $((OPTIND-1))

if ${background} ; then
    printf "Will start servers in background...\n"
fi

if ${all} ; then
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
         PANELS+=(${position})
    done < <(mysql --user=${MYSQL_USER} --password=${MYSQL_PASSWORD} --database=${MYSQL_DATABASE} --host=${MYSQL_HOST} --port=${MYSQL_PORT} -ss -e "SELECT position FROM Opt_MPMMapping")
else
    PANELS=("$@")
fi

count=$((${#PANELS[@]}))

if [[ ("$count" > 0) ]]; then
    printf "%s total panels selected\n" "$count"
    printf "Starting all servers...\n"

    if [[ -n ${LOCALIP} ]]; then
        IP=${LOCALIP}
    else
        IP="127.0.0.1"
    fi
    i=1
    for panel_num in "${PANELS[@]}"; do
        # Create temporary config file for each server to set the server endpoint
        # IP. Config files are created based on the template TemplateServerConfig.xml and
        # use sed to replace the string "URL_LOCATION" with the desired IP addr/port.
        # Temporary config files are named [panel_num].xml
        # They are deleted 30 sec after startup.
        config_filename="$panel_num$extension"
        endpoint_addr="opc.tcp://${IP}:${panel_num}"
        cp "TemplateServerConfig${extension}" "$config_filename"
        sed -i "s@URL_LOCATION@$endpoint_addr@g" "$config_filename"

        printf "(%d/%d) Starting server for panel %d at address %s.\n" "$i" "$count" "$panel_num" "$endpoint_addr"
        abspath=$(realpath ${config_filename})
        if ${background} ; then
            nohup ../sdk/bin/passerver ${panel_num} -c ${abspath} &>${panel_num}.log &
        else
            ../sdk/bin/passerver ${panel_num} -c ${abspath} &> ${panel_num}.log &
        fi
        ((i++))
    done

    if ! ${keep_config} ; then
        # Sleep for 15 seconds before deleting temporary config files
        # To give time for servers to start and read configuration
        sleep 15

        for panel_num in "${PANELS[@]}"; do
            config_filename="${panel_num}${extension}"
            rm "${config_filename}"
        done
    fi

    printf "Done.\n"

    if ! ${background}; then
        trap interrupt INT
        read -n1 -r -p "Press x to shut down..." key

        if [[ "${key}" = 'x' ]]; then
            interrupt
        fi
    fi
else
    usage
fi
