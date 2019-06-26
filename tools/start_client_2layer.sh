#!/usr/bin/env bash
usage() { echo "Usage: $0  [-a (all)] [panel number(s)] [-h]" 1>&2; exit 1; }

all=false
screen_name="client"
extension=".ini"

subclient_names=( "primary_upper" "primary_lower" "secondary" "other" )
subclient_ports=( "48011" "48012" "48013" "48014" )
subclient_array_names=( "PRIMARY_UPPER" "PRIMARY_LOWER" "SECONDARY" "OTHER" )

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
         PANELS+=($position)
    done < <(mysql --user=${MYSQL_USER} --password=${MYSQL_PASSWORD} --database=${MYSQL_DATABASE} --host=${MYSQL_HOST} --port=${MYSQL_PORT}-ss -e "SELECT position FROM Opt_MPMMapping")
else
    PANELS=("$@")
fi

printf "Received %d panel addresses.\n" "$((${#PANELS[@]}))"

printf "Reading from database...\n"

if [[ -z $MYSQL_USER || -z $MYSQL_PASSWORD || -z $MYSQL_DATABASE || -z $MYSQL_HOST || -z $MYSQL_PORT ]]; then
    echo "Database credentials not set - check $MYSQL_USER, $MYSQL_PASSWORD, $MYSQL_DATABASE, $MYSQL_HOST, $MYSQL_PORT"
    exit 1
fi

# Sort panels/servers into 4 groups (Secondary, Primary upper, Primary lower, Other)
PRIMARY_UPPER=()
PRIMARY_LOWER=()
SECONDARY=()
OTHER=()

while read -r position;
do
     if [[ mysql --user=${MYSQL_USER} --password=${MYSQL_PASSWORD} --database=${MYSQL_DATABASE} --host=${MYSQL_HOST} --port=${MYSQL_PORT}-ss -e "SELECT count(1) FROM Opt_MPMMapping where position='" + position +"'" ]]; then
         echo "No panel found with position $position"
         continue
     fi
     if [[ ${position:1:1} = "0" ]] || [[ ${position:0:1} = "3" ]]; then
        OTHER+=($position)
     elif [[ ${position:0:1} = "1" ]] && [[ ${position:0:1} = "1" || ${position:0:1} = "2" ]]; then
        PRIMARY_UPPER+=($position)
     elif [[ ${position:0:1} = "1" ]] && [[ ${position:0:1} = "3" || ${position:0:1} = "4" ]]; then
        PRIMARY_LOWER+=($position)
     elif [[ ${position:0:1} = "2" ]]; then
        SECONDARY+=($position)
     fi
done < <(PANELS)

# Start 4 separate subclients at 4 local addresses for the groups of panels

printf "Starting subclients...\n"

if ! screen -ls | grep -q ${screen_name}; then
    # -A for adaptive sizing
    # -dm for start in detached mode
    # -S to name the session
    screen -c helper/pas_serv_screen.config -AdmS ${screen_name}
fi

for i in {0..3}; do
    client_name=subclient_names[i]
    port=subclient_ports[i]
    config_filename="${client_name}${extension}"
    endpoint_addr="opc.tcp://127.0.0.1:$port"
    eval panel_list=\$$subclient_array_names[i]
    if [[ ${#panel_list[@]} -neq 0 ]]; then
        config_filename="${client_name}${extension}"
        endpoint_addr="opc.tcp://127.0.0.1:$port"
        screen -S ${screen_name} -X screen -t ${client_name}
        screen -S ${screen_name} -p ${client_name} -X stuff $"cp ../client/TemplateClientConfig${extension} ${config_filename}\n"
        screen -S ${screen_name} -p ${client_name} -X stuff "sed -i s@URL_LOCATION@$endpoint_addr@g ${config_filename}\n"
        printf "Starting %s client at address %s.\n" "${client_name}" "${endpoint_addr}"
        abspath=$(realpath ${config_filename})
        screen -S ${screen_name} -p ${client_name} -X stuff $"../sdk/bin/p2pasclient ${panel_list[*]}"
    else
        printf "No %s panels found. No subclient started. \n" "${client_name}"
    fi
done

# Now start high-level client connecting to all lower-level panels

printf "Starting top-level client...\n"

client_name="client"
port="48010"
config_filename="${client_name}${extension}"
endpoint_addr="opc.tcp://127.0.0.1:$port"

screen -S ${screen_name} -p ${client_name} -X stuff $"cp ../client/TemplateClientConfig${extension} ${config_filename}\n"
screen -S ${screen_name} -p ${client_name} -X stuff "sed -i s@URL_LOCATION@$endpoint_addr@g ${config_filename}\n"
printf "Starting %s client at address %s.\n" "${client_name}" "${endpoint_addr}"
abspath=$(realpath ${config_filename})
screen -S $screen_name -X stuff $"../sdk/bin/p2pasclient --top_level | tee ${HOME}/logs/p2pasclient_log_`date +%Y%m%d%H%M%S`.txt\n"


