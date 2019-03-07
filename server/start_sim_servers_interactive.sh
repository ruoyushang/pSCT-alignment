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

function interrupt() {    
  printf "\nKilling all servers and exiting...\n"
  killall -9 -q "passerver" 
  printf "Done.\n"
}

extension=".ini"

if $all ; then

printf "Reading from database...\n"

    PANELS=()
    while read -r position;
    do
         if [[ $position = "0" ]] || [[ $position = "1" ]] || [[ ${position:1:1} = "0" ]];
         then
             continue
         fi
         PANELS+=($position)
    done < <(mysql --user="CTAreadonly" --password="readCTAdb" --database="CTAonline" --host="remus.ucsc.edu" --port="3406" -ss -e "SELECT position FROM Opt_MPMMapping")
else
    PANELS=("$@")
fi

count=$((${#PANELS[@]}))

if [[ ("$count" > 0) ]]; then
    printf "%s total panels found\n" "$count"
    printf "Starting all servers...\n"

    i=1
    for panel_num in "${PANELS[@]}"; do
         config_filename="/app/pSCT-alignment/server/$panel_num$extension"
         endpoint_addr="opc.tcp://127.0.0.1:$panel_num"
         cp "TemplateServerConfig.ini" "$config_filename"
         sed -i "s@URL_LOCATION@$endpoint_addr@g" "$config_filename"
         printf "(%d/%d) Starting server for panel %d at address %s.\n" "$i" "$count" "$panel_num" "$endpoint_addr"
         abspath=$(realpath $config_filename)
         ../sdk/bin/passerver "${PANEL_MAP[$panel_num]}" "-c$abspath" > "/app/pSCT-alignment/server/${panel_num}.log" &
         ((i++))
    done

    sleep 10

    for panel_num in "${PANELS[@]}"; do
       config_filename="/app/pSCT-alignment/server/$panel_num$extension"
       rm "$config_filename"
    done

    printf "Done.\n"

    read -n1 -r -p "Press x to shut down..." key

    if [ "$key" = 'x' ]; then
        interrupt
    fi
else
    usage
fi

