FROM scientificlinux/sl:7
MAINTAINER dr2792@columbia.edu

RUN  yum -y update && yum erase cmake && yum -y install wget \
    yum install -y git make libxpm-dev \
    libxft-dev libxext-dev wget autotools-dev libtool automake autoconf \
    autoconf-archive bc binutils gcc\* \
    gfortran libssl-dev libpcre3-dev \
    xlibmesa-glu-dev libglew1.5-dev libftgl-dev \
    libfftw3-dev libcfitsio3-dev screen \
    graphviz-dev libavahi-compat-libdnssd-dev \
    libldap2-dev libxml2-dev libkrb5-dev \
    libgsl0-dev libqt4-dev libgsl0-dev libgsl0-dbg \
    python3-dev python3-pip git gdb libudev-dev libudev1 \
    libtinyxml-dev libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev libglib2.0-dev \
    libgirepository1.0-dev libusb-1.0-0-dev libzip-dev \
    uvcdynctrl python-setuptools libpcap-dev \
    libaudit-dev libnotify-dev autoconf intltool \
    gtk-doc-tools python3-setuptools libX11-devel.x86_64 \
    libXpm.x86_64 libXpm-devel.x86_64 libXft.x86_64 \
    libXft-devel.x86_64 libXext.x86_64 libXext-devel.x86_64 && \
    yum groupinstall -y "X Window System" && \
    yum install -y gstreamer1\* openssh-server\
    libxml2 libpcap0.8 libaudit1 libnotify4 gtk-doc \
    python3-pyqt5 python3-gi openssl-devel.x86_64 \
    libusbmuxd-devel.x86_64 libusb.x86_64 libusbx.x86_64 \
    libusb-devel.x86_64 && yum clean all


RUN mkdir software
WORKDIR /software
RUN mkdir tarballs ROOT_build mysql_5.7 mysqlcppconn_build mysqlcppconn opencv_build /var/run/sshd /root/logs/

RUN echo 'root:pass' | chpasswd
RUN sed -i 's/PermitRootLogin prohibit-password/PermitRootLogin yes/' /etc/ssh/sshd_config

ENV NOTVISIBLE "in users profile"
RUN echo "export VISIBLE=now" >> /etc/profile
RUN /usr/bin/ssh-keygen -A

ENV NTHREADS 4

# Install gcc-4.9.2
#RUN wget --directory-prefix=tarballs http://mirrors.concertpass.com/gcc/releases/gcc-4.9.2/gcc-4.9.2.tar.gz \
#    && tar -xzf tarballs/gcc-4.9.2.tar.gz && cd gcc-4.9.2 && mkdir objdir && cd objdir \
#    && ../configure --enable-languages=c,c++,fortran --disable-multilib && make \
#    && make install

# Install cmake 3.9.1
RUN wget https://cmake.org/files/v3.9/cmake-3.9.1-Linux-x86_64.sh \
    && chmod +x cmake-3.9.1-Linux-x86_64.sh \
    && ./cmake-3.9.1-Linux-x86_64.sh --prefix=/usr/local/ --exclude-subdir

# Install cfitsio
RUN wget --directory-prefix=tarballs https://heasarc.gsfc.nasa.gov/FTP/software/fitsio/c/cfitsio3410.tar.gz \
    && tar -xzf tarballs/cfitsio3410.tar.gz && cd /software/cfitsio && ./configure \
    --prefix=/usr/local/ && make -j${NTHREADS} && make install

# Install ccfits
RUN wget --directory-prefix=tarballs https://heasarc.gsfc.nasa.gov/fitsio/CCfits/CCfits-2.5.tar.gz \
    && tar -zxf tarballs/CCfits-2.5.tar.gz \
    && cd /software/CCfits \
    && ./configure --with-cfitsio=/software/cfitsio \
    && make -j${NTHREADS} \
    && make install

# Install ROOT
ENV ROOT_V 6.10.04
#ADD root_v6.10.04.source.tar.gz /software/
#WORKDIR /software/ROOT_build
#RUN cmake -Dgsl_shared=YES -Droofit=YES ../root-${ROOT_V}/ \
#    && cmake --build . -- -j${NTHREADS} \
#    && cmake --build . --target install \
#    && rm -rf /software/root-${ROOT_V}
WORKDIR /software
RUN wget --directory-prefix=tarballs https://root.cern.ch/download/root_v6.10.04.Linux-centos7-x86_64-gcc4.8.tar.gz \
    && tar -zxf tarballs/root_v6.10.04.Linux-centos7-x86_64-gcc4.8.tar.gz

# initialize ROOT environment variables
#ENV ROOTSYS "/software/ROOT_build"
ENV ROOTSYS "/software/root"
ENV PATH "${PATH}:${ROOTSYS}/bin"
ENV LD_LIBRARY_PATH "${LD_LIBRARY_PATH}:${ROOTSYS}/lib"

# Install BOOST
ENV BOOST_V 1.67.0
WORKDIR /software
RUN wget --directory-prefix=tarballs https://sourceforge.net/projects/boost/files/boost/${BOOST_V}/boost_1_67_0.tar.gz \
    && tar -zxf tarballs/boost_1_67_0.tar.gz -C /usr/local/
WORKDIR /usr/local/boost_1_67_0
RUN ./bootstrap.sh \
    && ./b2 || true
RUN ./b2 install || true

# Install Mysql and Mysql-Connector
WORKDIR /software
RUN wget --directory-prefix=tarballs https://dev.mysql.com/get/Downloads/MySQL-5.7/mysql-5.7.25-linux-glibc2.12-x86_64.tar.gz \
    && tar -zxf tarballs/mysql-5.7.25-linux-glibc2.12-x86_64.tar.gz -C ./mysql_5.7 --strip-components=1
RUN wget --directory-prefix=tarballs https://dev.mysql.com/get/Downloads/Connector-C++/mysql-connector-c++-1.1.12.tar.gz \
    && tar -zxf tarballs/mysql-connector-c++-1.1.12.tar.gz -C ./mysqlcppconn --strip-components=1

ENV MYSQL_DIR /software/mysql_5.7
ENV PATH "${PATH}:${MYSQL_DIR}/bin"
ENV LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${MYSQL_DIR}/lib/:/usr/local/lib:/usr/local/lib64/:/lib64"
ENV PKG_CONFIG_PATH="${PKG_CONFIG_PATH}:${MYSQL_DIR}/lib/pkgconfig:/usr/local/lib/pkgconfig"
WORKDIR /software/mysqlcppconn_build
RUN cmake -DBOOST_ROOT=/usr/local/boost_1_67_0/ ../mysqlcppconn \
    && cmake --build . -- -j${NTHREADS} \
    && cmake --build . --target install
COPY mysqlcppconn.pc /usr/local/lib/pkgconfig

# Install OpenCV
WORKDIR /software
RUN git clone https://github.com/opencv/opencv.git \
    && cd opencv \
    && git checkout 3.4.1 \
    && git checkout -b 3.4.1
WORKDIR /software/opencv_build
RUN cmake ../opencv \
    && cmake --build . -- -j${NTHREADS} \
    && cmake --build . --target install

# Install Eigen3
WORKDIR /software
RUN wget --directory-prefix=tarballs http://bitbucket.org/eigen/eigen/get/3.3.7.tar.gz \
    && tar -zxf tarballs/3.3.7.tar.gz
WORKDIR /software/eigen-eigen-323c052e1731
RUN mkdir build \
    && cd build \
    && cmake .. \
    && make -j${NTHREADS} \
    && make install

# Install nlopt
WORKDIR /software
RUN git clone https://github.com/stevengj/nlopt.git
WORKDIR /software/nlopt
RUN git checkout v2.5.0 \
    && git checkout -b v2.5.0 \
    && mkdir build \
    && cd build \
    && cmake .. \
    && make \
    && make install
ENV PKG_CONFIG_PATH="${PKG_CONFIG_PATH}:/usr/local/lib64/pkgconfig"

# Install aravis
WORKDIR /software
RUN git clone https://github.com/AravisProject/aravis.git \
    && cd aravis \
    && git checkout ARAVIS_0_4_0 \
    && git checkout -b ARAVIS_0_4_0
WORKDIR /software/aravis
RUN ./autogen.sh \
    && make -j${NTHREADS} \
    && make install \
    && ln -s /lib64/libusb-1.0.so.0 ${ROOTSYS}/lib/

# Install Tiscamera
#WORKDIR /software/
#RUN git clone --recursive https://github.com/TheImagingSource/tiscamera.git \
#    && cd tiscamera && git checkout v-tiscamera-0.10.0 \
#    && git checkout -b v-tiscamera-0.10.0 && mkdir build \
#    && cd build

# Install Robast
WORKDIR /software/
RUN git clone https://github.com/ROBAST/ROBAST.git ROBAST
WORKDIR /software/ROBAST
RUN git checkout v2.4.4 \
    && git checkout -b v2.4.4 \
    && make
ENV LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:/software/ROBAST/"
COPY ROBAST.pc /usr/local/lib/pkgconfig

WORKDIR /software/
RUN git clone https://github.com/gabime/spdlog.git \
    && mkdir -p /software/spdlog/build && cd /software/spdlog/build/ \
    && cmake .. \
    && cmake --build . -- -j${NTHREADS} \
    && cmake --build . --target install

WORKDIR /software
RUN git clone https://github.com/doxygen/doxygen.git \
    && yum -y install graphviz flex.x86_64 strip bison \
    && mkdir /software/doxygen/build && cd /software/doxygen/build \
    && cmake -G "Unix Makefiles" .. \
    && make && make install

COPY bash_profile /root/.bash_profile
COPY screenrc /root/.screenrc
WORKDIR /app

#CMD ["/usr/sbin/sshd", "-D"]
