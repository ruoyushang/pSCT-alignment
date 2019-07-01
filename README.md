# pSCT-alignment
Alignment software for pSCT

# Installation

Remember to call source thisroot to set ROOT environment variables correctly.

# Dependencies

* OpenCV 4.0
* ROOT 6.10.04
* MySQL 5.5.61

# Usage
To build for SIMMODE,
```bash
cd ./sdk/
./buildServer.sh -cSIMMODE
./buildClient.sh -cSIMMODE
```
If using another environment, check the help menu for each build script. This will install binaries to `./sdk/bin`, and you can set up SIMMODE instances by creating servers in `./server/start_sim_servers.sh -a` and `./client/start_sim_client.sh`. Make sure to set environment variable `LOCALIP` to the host address of the local SIMMODE instance - if using docker, this is the host discovered below in the docker section, but otherwise usually set to `127.0.0.1`.


# Docker
Build docker image by going to docker/ directory. In it are several files that need to be copied on to image, and the Dockerfile. Use the following build command to create both the alignment image and mysql on your computer:
```bash
docker-compose build .
``` 
This build process takes about 1hr to complete since it is currently a single monolithic OS+dependencies image, about 11gb in size. We will develop further to containerize subcomponents and reduce the image size.

Once built, run the set of images together with 

```bash
docker-compose up -d
``` 
This will start the mysql and alignment container. To connect, do not use `docker run <options>` like in previous instances but instead ssh directly to the alignment container.
```bash
ssh -p 3022 root@0.0.0.0
```
This will log you into the container. The password is defined inside the Dockerfile image = `pass`. From here, you can compile the alignment git repository from `/app/pSCT-alignment/sdk` with the build shell scripts.

Before building the binaries inside the container (after `docker-compose up -d`, but before `./buildClient.sh -cSIMMODE`), check that the host address is correct for docker's internet container network:

```bash
docker inspect -f '{{.Name}} - {{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' $(docker ps -aq)
```
This will yield IP address for main and mysql. Copy the value for main and export into main while using the server or client:

```bash
export LOCALIP="value"
```
Make sure this IP address is also in `sdk/bin/PasServerConfig.xml` line 137, following <UaServerConfig><UaEndpoint><URL>.
  
# Generating documentation with Doxygen

To re-generate all doxygen documentation, from the root directory (containing the doxygen/ directory), run:

```bash
doxygen doxygen/Doxyfile
```

NOTE: It is required that the command is executed from the root directory for the files to be searched correctly.

The generated html and LaTeX documentation will be located at:

```bash
doxygen/html
doxygen/latex
```
