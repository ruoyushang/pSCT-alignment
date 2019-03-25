# Script to start simulation mode servers.
# Receives either a list of panel (position) numbers, or the option -a for all
# when "all" is selected, it will read all panels from the database except
# the test panels 0, 1, and any optical table panels (1001, 2001)

function interrupt() {    
  printf "\nKilling all servers and exiting...\n"
  killall -9 -q "passerver" 
  printf "Done.\n"
}

# NOTE: DB access information is hardcoded

usage() { echo "Usage: $0  [-a (all)] [-h (help)] [-b (background)] [panel number(s)]" 1>&2; exit 1; }

all=false
background=false
extension=".ini"

while getopts "ahb" o; do
    case "${o}" in
    a) all=true ;;
    b) background=true ;;
    h) usage ;;
    esac
done

shift $((OPTIND-1))

if ${background} ; then
    printf "Will start servers in background...\n"
fi

if ${all} ; then
    while read -r position;
    do
         if [[ ${position} = "0" ]] || [[ ${position} = "1" ]] || [[ ${position:1:1} = "0" ]];
         then
             continue
         fi
         PANEL_MAP[$position]=${ip_address}
    done < <(mysql --user="CTAreadonly" --password="readCTAdb" --database="CTAonline" --host="remus.ucsc.edu" --port="3406" -ss -e "SELECT position, mpcb_ip_address FROM Opt_MPMMapping")
else
    PANELS=("$@")
fi

count=$((${#PANELS[@]}))

if [[ ("$count" > 0) ]]; then
    printf "%s total panels selected\n" "$count"
    printf "Starting all servers...\n"

    IP=${LOCALIP}
    i=1
    for panel_num in "${PANELS[@]}"; do
         # Create temporary config file for each server to set the server endpoint
         # IP. Config files are created based on the template TemplateServerConfig.xml and
         # use sed to replace the string "URL_LOCATION" with the desired IP addr/port.
         # Temporary config files are named [panel_num].xml
         # They are deleted 30 sec after startup.
         config_filename="$panel_num$extension"
         endpoint_addr="opc.tcp://127.0.0.1:$panel_num"
         cp "TemplateServerConfig${extension}" "$config_filename"
         sed -i "s@URL_LOCATION@$endpoint_addr@g" "$config_filename"

         printf "(%d/%d) Starting server for panel %d at address %s.\n" "$i" "$count" "$panel_num" "$endpoint_addr"
         abspath=$(realpath ${config_filename})
         if $background ; then
           nohup ../sdk/bin/passerver "${panel_num}" "-c $abspath" &>"${panel_num}.log" &
         else
           ../sdk/bin/passerver "${panel_num}" "-c" "$abspath" >"${panel_num}.log" &
         fi
         ((i++))
    done

    # Sleep for 30 seconds before deleting temporary config files
    # To give time for servers to start and read configuration
    #sleep 30

    for panel_num in "${PANELS[@]}"; do
       config_filename="$panel_num$extension"
       #rm "$config_filename"
    done

    printf "Done.\n"

    if ! $background ; then
      trap interrupt INT
      read -n1 -r -p "Press x to shut down..." key

      if [ "$key" = 'x' ]; then
          interrupt
      fi
    fi
else
    usage
fi
