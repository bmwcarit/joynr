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
# install boost
###################################################
RUN dnf update -y \
	&& dnf install -y \
	boost \
	boost-devel \
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
# install gcovr for code coverage reports
###################################################
RUN dnf update -y \
	&& dnf install -y \
	python-pip \
	&& dnf clean all \
	&& pip install gcovr

###################################################
# install lcov
###################################################
RUN dnf update -y \
	&& dnf install -y \
	lcov \
	&& dnf clean all

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
# install googletest & googlemock
###################################################

RUN cd /opt \
    && git clone https://github.com/google/googletest.git \
    && cd googletest \
    && git checkout ddb8012e \
    && mkdir build \
    && cd build \
    && cmake .. \
    && make install -j"$(nproc)" \
    && cd /opt/ \
    && rm -rf googletest

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
# install DLT
###################################################

RUN cd /opt \
    && git clone http://git.projects.genivi.org/dlt-daemon.git \
    && cd dlt-daemon \
    && git checkout v2.15.0 \
    && mkdir build \
    && cd build \
    && cmake .. -DWITH_DLT_DBUS=OFF -DWITH_DLT_TESTS=OFF -DWITH_DLT_EXAMPLES=OFF \
    && make install -j"$(nproc)" \
    && cd /opt/ \
    && rm -rf dlt-daemon

# DLT installs itself in /usr/local/lib64
ENV PKG_CONFIG_PATH $PKG_CONFIG_PATH:/usr/local/lib64/pkgconfig

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
