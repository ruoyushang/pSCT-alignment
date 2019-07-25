#!/usr/bin/env bash
usage() { echo "Usage: $0  [-a (all)] [panel number(s)] [-h]" 1>&2; exit 1; }

all=false
screen_name="sim_client"
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

if [[ -z $MYSQL_USER || -z $MYSQL_PASSWORD || -z $MYSQL_DATABASE || -z $MYSQL_HOST || -z $MYSQL_PORT ]]; then
    echo "Database credentials not set - check $MYSQL_USER, $MYSQL_PASSWORD, $MYSQL_DATABASE, $MYSQL_HOST, $MYSQL_PORT"
    exit 1
fi

if $all ; then

printf "Reading from database...\n"

    PANELS=()
    while read -r position;
    do
         PANELS+=($position)
    done < <(mysql --user=${MYSQL_USER} --password=${MYSQL_PASSWORD} --database=${MYSQL_DATABASE} --host=${MYSQL_HOST} --port=${MYSQL_PORT}-ss -e "SELECT position FROM Opt_MPMMapping")
else
    PANELS=("$@")
fi

printf "Received %d panel addresses.\n" "$((${#PANELS[@]}))"

# Sort panels/servers into 4 groups (Secondary, Primary upper, Primary lower, Other)
PRIMARY_UPPER=()
PRIMARY_LOWER=()
SECONDARY=()
OTHER=()

for position in ${PANELS[@]}; 
do
    count=`mysql --user=${MYSQL_USER} --password=${MYSQL_PASSWORD} --database=${MYSQL_DATABASE} --host=${MYSQL_HOST} --port=${MYSQL_PORT} -ss -e "SELECT EXISTS(SELECT 1 FROM Opt_MPMMapping where position=$position)"`
    if [[ "$count" -eq "0" ]]; then
         echo "No panel found with position $position"
         continue
     fi
     if [[ "${position:1:1}" -eq "0" ]] || [[ "${position:0:1}" -eq "3" ]]; then
        OTHER+=($position)
     elif [[ "${position:0:1}" -eq "1" ]] && [[ "${position:1:1}" -eq "1" || "${position:1:1}" -eq "2" ]]; then
        PRIMARY_UPPER+=($position)
     elif [[ "${position:0:1}" -eq "1" ]] && [[ "${position:1:1}" -eq "3" || "${position:1:1}" -eq "4" ]]; then
        PRIMARY_LOWER+=($position)
     elif [[ "${position:0:1}" -eq "2" ]]; then
        SECONDARY+=($position)
     fi
 done

# Start 4 separate subclients at 4 local addresses for the groups of panels

printf "Starting subclients...\n"

if ! screen -ls | grep -q ${screen_name}; then
    # -A for adaptive sizing
    # -dm for start in detached mode
    # -S to name the session
    screen -c helper/pas_serv_screen.config -AdmS ${screen_name}
fi

clients_to_connect=""
for i in {0..3}; do
    client_name=${subclient_names[i]}
    port=${subclient_ports[i]}
    config_filename="${client_name}${extension}"
    endpoint_addr="opc.tcp://127.0.0.1:$port"
    subclient_array_name=${subclient_array_names[i]}[@]
    panel_list=("${!subclient_array_name}")
   
    printf "Received %d %s panel addresses.\n" "${#panel_list[@]}" "$client_name"

    if [[ "${#panel_list[@]}" -ne "0" ]]; then
        config_filename="${client_name}${extension}"
        endpoint_addr="opc.tcp://127.0.0.1:$port"
        screen -S ${screen_name} -X screen -t ${client_name}
        screen -S ${screen_name} -p ${client_name} -X stuff $"cp ../client/TemplateClientConfig${extension} ${config_filename}\n"
        screen -S ${screen_name} -p ${client_name} -X stuff "sed -i s@URL_LOCATION@$endpoint_addr@g ${config_filename}\n"
        printf "Starting %s client at address %s.\n" "${client_name}" "${endpoint_addr}"
        abspath=$(realpath ${config_filename})
        clients_to_connect+="$client_name "
        screen -S ${screen_name} -p ${client_name} -X stuff $"../sdk/bin/p2pasclient ${panel_list[*]} -m subclient -c $abspath \n"
    else
        printf "No %s panels found. No subclient started. \n" "${client_name}"
    fi
done

# Now start high-level client connecting to all lower-level panels
printf "Sleeping to give subclients time to start...\n"

sleep 30

printf "Starting top-level client...\n"

client_name="client"
port="48015"
config_filename="${client_name}${extension}"
endpoint_addr="opc.tcp://10.0.1.13:$port"

screen -S ${screen_name} -X screen -t ${client_name}
screen -S ${screen_name} -p ${client_name} -X stuff $"cp ../client/TemplateClientConfig${extension} ${config_filename}\n"
screen -S ${screen_name} -p ${client_name} -X stuff "sed -i s@URL_LOCATION@$endpoint_addr@g ${config_filename}\n"
printf "Starting %s client at address %s.\n" "${client_name}" "${endpoint_addr}"
abspath=$(realpath ${config_filename})
screen -S ${screen_name} -X stuff $"../sdk/bin/p2pasclient $clients_to_connect -m client -c $abspath\n"


