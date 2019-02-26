# pSCT-alignment
Alignment software for pSCT

# Installation

Remember to call source thisroot to set ROOT environment variables correctly.

# Dependencies

* OpenCV 4.0
* ROOT 6.10.04
* MySQL 5.5.61

# Usage

# Docker
Build docker image by going to docker/ directory. In it are two files that need to be copied on to image, and the Dockerfile. Use the following build command to create an image on your computer:
```bash
docker build .
``` 
It is recommended that you add a tag to label that image with `-t yourname/alignment` for example. This build process takes about 1hr to complete since it is currently a single monolithic OS+dependencies image, about 11gb in size. We will develop further to containerize subcomponents and reduce the image size.

Once built, run the image using volume building. Docker creates a read only container from your image, so development is difficult because you can't save progress on the image. Instead, you can work on a local directory (as opposed to the container itself) and build against the container. Binding allows you to point to the local directory from an open directory on the container.
Use the following run commmand to do this, making sure to use your own local directory first, followed by /app, which is the directory exposed during build. 
```bash
docker run -it --run -v/path/to/your/local/directory:/app yourname/alignment
```

It is recommended that /path/to/your/local/directory is the top directory where this git repo live, so that within the container terminal you can navigate to `/app/pSCT-alignment/sdk/bin/`. The image is identified here with the tag specified above, but you can also explicitly point to the ImageID from the list in `docker images`. 