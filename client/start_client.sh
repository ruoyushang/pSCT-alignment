usage() { echo "Usage: $0  [-a (all)] [panel number(s)] [-h]" 1>&2; exit 1; }

all=false

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

printf "Starting client...\n"

opcua-client &>> /dev/null &

../sdk/bin/p2pasclient "${PANELS[@]}"
