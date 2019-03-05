# Script to kill all processes named 'passerver'
# NOTE: directly kills processes, use at your own risk

printf "\nKilling all servers and exiting...\n"
killall -9 -q "passerver"
printf "Done.\n"
