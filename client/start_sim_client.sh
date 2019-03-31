#!/usr/bin/env bash
printf "Reading from database...\n"

declare -a panel_numbers
while read -r position;
do
  if [[ $position = "0" ]] || [[ ${position:1:1} = "0" ]] || [[ ${position:0:1} = "1" ]]; then
      continue
  fi
  panel_numbers+=("$position")
done < <(mysql --user=${MYSQL_USER} --password=${MYSQL_PASSWORD} --database=${MYSQL_DATABASE} --host=${MYSQL_HOST} --port=${MYSQL_PORT} -ss -e "SELECT position FROM Opt_MPMMapping")

count=$((${#panel_numbers[@]}))
printf "%s total panel servers found in database\n" "$count"
printf "Starting client...\n"

opcua-client &>> /dev/null &

../sdk/bin/p2pasclient "${panel_numbers[@]}"
