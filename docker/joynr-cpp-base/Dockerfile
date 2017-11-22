FROM joynr-base:latest

ENV PKG_CONFIG_PATH /usr/local/lib/pkgconfig:$PKG_CONFIG_PATH
COPY 0001-do-not-pass-empty-connection-to-async_accept-in-serv.patch /tmp/0001-do-not-pass-empty-connection-to-async_accept-in-serv.patch

###################################################
# enable gold linker
###################################################
RUN  update-alternatives  --set ld /usr/bin/ld.gold

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
# install spdlog
###################################################

RUN cd /opt \
    && git clone https://github.com/gabime/spdlog.git \
    && cd spdlog \
    && git checkout 4fcde3b \
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
    && git apply /tmp/0001-do-not-pass-empty-connection-to-async_accept-in-serv.patch \
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
    && git clone https://github.com/GENIVI/dlt-daemon \
    && cd dlt-daemon \
    && git checkout v2.15.0 \
    && sed -i 's/libdir=${exec_prefix}\/lib/libdir=@CMAKE_INSTALL_FULL_LIBDIR@/' automotive-dlt.pc.in \
    && sed -i 's/includedir=${exec_prefix}\/include/includedir=@CMAKE_INSTALL_FULL_INCLUDEDIR@/' automotive-dlt.pc.in \
    && mkdir build \
    && cd build \
    && cmake .. -DWITH_DLT_DBUS=OFF -DWITH_DLT_TESTS=OFF -DWITH_DLT_EXAMPLES=OFF \
    && make install -j"$(nproc)" \
    && cd /opt/ \
    && rm -rf dlt-daemon \
    && echo '/usr/local/lib64' > /etc/ld.so.conf.d/dlt.conf


###################################################
# install flatbuffers
###################################################

RUN export FLATBUFFERS_VERSION=v1.6.0 \
        && cd /tmp \
        && git clone https://github.com/google/flatbuffers.git \
        && cd flatbuffers \
        && git checkout $FLATBUFFERS_VERSION \
        && mkdir build \
        && cd build \
        && cmake .. -DFLATBUFFERS_BUILD_TESTS=OFF \
        && make install -j"$(nproc)" \
        && rm -rf /tmp/flatbuffers


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
