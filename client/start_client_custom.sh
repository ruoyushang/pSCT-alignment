#!/usr/bin/env bash
usage() { echo "Usage: $0  [list_name (from tools/panel_lists)] [-h]" 1>&2; exit 1; }

list_name=$1

while getopts "h" o; do
    case "${o}" in
    h) usage ;;
    esac
done

printf "Starting client...\n"

../sdk/bin/p2pasclient `cat ../tools/panel_lists/$list_name` | tee "$HOME/logs/p2pasclient_log_`date +%Y%m%d%H%M%S`.txt"
