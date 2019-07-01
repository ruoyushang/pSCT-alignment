#!/usr/bin/env bash
usage() { echo "Usage: $0  [list_name (from tools/panel_lists)] [-h]" 1>&2; exit 1; }

list_name=$1

while getopts "h" o; do
    case "${o}" in
    h) usage ;;
    esac
done

printf "Starting 2-layer client...\n"

./start_client_2layer.sh `cat ../tools/panel_lists/$list_name` 
