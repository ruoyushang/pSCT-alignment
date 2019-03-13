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
If using another environment, check the help menu for each build script. This will install binaries to `./sdk/bin`, and you can set up SIMMODE instances by creating servers in `./server/start_sim_servers.sh -a` and `./client/start_sim_client.sh`. 


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
This will start the mysql and alignment container. To connect, do not use `docker run <options>` but instead ssh directly to the alignment container.
```bash
ssh -p 3022 root@0.0.0.0
```
This will log you into the container. The password is defined inside the Dockerfile image - `pass`. From here, you can compile the alignment git repository from `/app/pSCT-alignment/sdk` with the build shell scripts.

 