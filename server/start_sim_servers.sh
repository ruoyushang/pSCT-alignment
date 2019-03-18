#!/usr/bin/env bash
trap interrupt INT

usage() { echo "Usage: $0  [-a (all)] [panel number(s)] [-h]" 1>&2; exit 1; }

all=false

while getopts "ah" o; do
    case "${o}" in
    a) all=true ;;
    h) usage ;;
    esac
done

shift $((OPTIND-1))

extension=".ini"

if ${all} ; then

printf "Reading from database...\n"
declare -A PANEL_MAP

    while read -r position ip_address;
    do
         if [[ ${position} = "0" ]] || [[ ${position} = "1" ]] || [[ ${position:1:1} = "0" ]];
         then
             continue
         fi
         PANEL_MAP[$position]=${ip_address}
    done < <(mysql --user="CTAreadonly" --password="readCTAdb" --database="CTAonline" --host="remus.ucsc.edu" --port="3406" -ss -e "SELECT position, mpcb_ip_address FROM Opt_MPMMapping")
#else
#    PANELS=("$@")
fi

count=$((${#PANEL_MAP[@]}))

if [[ ("$count" > 0) ]]; then
    printf "%s total panels found\n" "$count"
    printf "Starting all servers...\n"

    IP=${LOCALIP}
    i=1
    for panel_num in "${!PANEL_MAP[@]}"; do
         config_filename="$panel_num$extension"
         endpoint_addr="opc.tcp://${IP}:$panel_num"
         cp "TemplateServerConfig${extension}" "$config_filename"
         sed -i "s@URL_LOCATION@$endpoint_addr@g" "$config_filename"
         printf "(%d/%d) Starting server for panel %d at address %s.\n" "$i" "$count" "$panel_num" "$endpoint_addr"
         abspath=$(realpath ${config_filename})
         printf "../sdk/bin/passerver %s:%s -c%s > %s.log &\n" "${PANEL_MAP[$panel_num]}" "$panel_num" "$abspath" "${panel_num}"
         ../sdk/bin/passerver "${PANEL_MAP[$panel_num]}:$panel_num" "-c$abspath" > "${panel_num}.log" &
         ((i++))
    done

    sleep 10

    for panel_num in "${!PANEL_MAP[@]}"; do
       config_filename="$panel_num$extension"
#       rm "$config_filename"
    done

    printf "Done.\n"
else
    usage
fi

