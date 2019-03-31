#!/usr/bin/env bash
  printf "\nKilling all servers and exiting...\n"
  killall -9 -q "passerver" 
  printf "Done.\n"
