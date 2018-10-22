trap interrupt INT

function interrupt() {
  printf "Killing all servers and exiting...\n"
  killall -9 -q "passerver" 
  printf "Done.\n"
}

extension=".xml"

printf "Reading from database...\n"

declare -A PANEL_MAP
while read -r position ip_address;
do
    PANEL_MAP[$position]=$ip_address
done < <(mysql --user="CTAreadonly" --password="readCTAdb" --database="CTAonline" --host="remus.ucsc.edu" --port="3406" -ss -e "SELECT position, mpcb_ip_address FROM Opt_MPMMapping")

count=${#PANEL_MAP[@]}
printf "%s total panels and IPs found in database\n" "$count"
printf "Starting all servers...\n"

i=1
for panel_num in "${!PANEL_MAP[@]}"; do
     config_filename="$panel_num$extension"
     endpoint_addr="opc.tcp://10.0.1.13:$panel_num"
     cp "TemplateServerConfig.xml" "$config_filename"
     sed -i "s@[URL_LOCATION]@$endpoint_addr@g" "$config_filename"
     printf "(%d/%d) Starting server for panel %d at address %s.\n" "$i" "$count" "$panel_num" "$endpoint_addr"
     abspath=$(realpath config_filename)
     ../sdk/bin/passerver "${PANEL_MAP[$panel_num]}" "-c $abspath" >/dev/null &
     rm "$config_filename"
     ((i++))
done

printf "Done.\n"

read -n1 -r -p "Press x to shut down..." key

if [ "$key" = 'x' ]; then
    interrupt
fi


