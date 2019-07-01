#! /usr/bin/env bash

panel_list=$1
for pos in `cat ~/repos/bryan/pSCT-alignment/tools/panel_lists/$panel_list`; do
    ~/p2pas/passerver_tools/get_panel_current.sh $pos & 
done
