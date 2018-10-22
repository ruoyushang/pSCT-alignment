trap interrupt INT

extension=".xml"

printf "Reading from database..."

declare -A panel_map
mysql -user=CTAreadonly -password=readCTAdb --database=CTAonline --host=remus.ucsc.edu --port=3406 "SELECT position, mpcb_ip_address FROM Opt_MPMMapping" | while read -r position ip_address;
do
    panel_map[$position]=$ip_address
done

count=${#panel_map[@]}
printf "%s total panels and IPs found in database" "$count"
printf "Starting all servers..."

i=1
for panel_num in "${!panel_map[@]}"; do
     config_filename="$panel_num$extension"
     endpoint_addr="opc.tcp://10.0.1.13:$panel_num"
     cp "TemplateServerConfig.xml" "$config_filename"
     sed -i "s/[URL_LOCATION]/$endpoint_addr/g" "$config_filename"
     printf "(%d/%d) Starting server for panel %d at address %s" "$i" "$count" "$panel_num" "$endpoint_addr"
     abspath=$(realpath config_filename)
     ../sdk/bin/passerver "${panel_map[$panel_num]}" "-c $abspath" &
     ((i++))
done

rm "$config_filename"
printf "Done."

function interrupt() {
  printf "Killing all servers and exiting..."
  pkill -f "passerver"
  printf "Done."
}
