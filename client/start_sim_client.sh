printf "Reading from database...\n"

declare -a panel_numbers
while read -r position;
do
  panel_numbers+=("$position")
done < <(mysql --user="CTAreadonly" --password="readCTAdb" --database="CTAonline" --host="remus.ucsc.edu" --port="3406" -ss -e "SELECT position FROM Opt_MPMMapping")

count=${#panel_numbers[@]}
printf "%s total panel servers found in database\n" "$count"
printf "Starting client...\n"

../sdk/bin/p2pasclient "${myArray[*]}"
