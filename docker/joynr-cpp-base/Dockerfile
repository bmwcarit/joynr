FROM joynr-base:latest

ENV PKG_CONFIG_PATH /usr/local/lib/pkgconfig:$PKG_CONFIG_PATH

###################################################
# install qt
###################################################
RUN dnf update -y \
	&& dnf install -y \
	qt5-qtbase \
	qt5-qtbase-devel \
	qt5-qtwebsockets \
	qt5-qtwebsockets-devel \
	&& dnf clean all

###################################################
# install clang 3.5 for formatting
###################################################
RUN cd /tmp/ \
    && wget http://llvm.org/releases/3.5.0/clang+llvm-3.5.0-x86_64-fedora20.tar.xz \
    && tar -xf clang+llvm-3.5.0-x86_64-fedora20.tar.xz \
    clang+llvm-3.5.0-x86_64-fedora20/bin/clang-format \
    --strip-components=2 \
    && mv clang-format /usr/local/bin/ \
    && rm -f clang+llvm-3.5.0-x86_64-fedora20.tar.xz

###################################################
# install rpm-build required by CMake / CPack
###################################################
RUN dnf update -y \
	&& dnf install -y \
	rpm-build \
	&& dnf clean all

###################################################
# install jsmn
###################################################

RUN cd /opt \
    && git clone https://github.com/zserge/jsmn.git \
    && cd jsmn \
    && git checkout 572ace5 \
    && CFLAGS=-fPIC make \
    && cp libjsmn.a /usr/local/lib \
    && cp jsmn.h /usr/local/include \
    && cd /opt/ \
    && rm -rf jsmn

###################################################
# install spdlog
###################################################

RUN cd /opt \
    && git clone https://github.com/gabime/spdlog.git \
    && cd spdlog \
    && git checkout e91e1b8 \
    && mkdir build \
    && cd build \
    && cmake .. \
    && make install -j"$(nproc)" \
    && cd /opt/ \
    && rm -rf spdlog

###################################################
# install websocket++
###################################################

RUN cd /opt \
    && git clone https://github.com/zaphoyd/websocketpp.git \
    && cd websocketpp \
    && git checkout 378437aecdcb1dfe62096ffd5d944bf1f640ccc3 \
    && mkdir build \
    && cd build \
    && cmake .. \
    && make install -j"$(nproc)" \
    && cd /opt/ \
    && rm -rf websocketpp

###################################################
# Copy build scripts
###################################################
COPY scripts /data/scripts

###################################################
# setup build environment
###################################################
RUN mkdir -p /home/joynr/
RUN echo "source /data/scripts/bashrc.sh" >> /home/joynr/.bashrc
RUN mkdir /home/joynr/build
RUN date -R > /data/timestamp
