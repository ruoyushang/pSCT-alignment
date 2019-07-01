#!/bin/bash

ip_address=$1

# Copy over spdlog and google benchmark repos
ssh -n -q root@$ip_address "mkdir -p software"

scp -r ~/home/user/repos/bryan/spdlog root@$ip_address:~/software
scp -r ~/home/user/repos/bryan/benchmark root@$ip_address:~/software

# Build and install benchmark
ssh -n -q root@$ip_address "cd ~/software/benchmark/build && cmake .. && make && make install"

# Build and install spdlog
ssh -n -q root@$ip_address "cd ~/software/spdlog && cmake && make && make install"

# Copy over new passerver binary
scp -q ~/home/user/repos/bryan/passerver root@$ip_address:~/opcua/sdk/bin/
scp -q ~/home/user/repos/bryan/pSCT-alignment/server/start_server.sh root@$ip_address:~/opcua/
#echo " DONE!"

exit 0
