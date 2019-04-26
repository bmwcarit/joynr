FROM joynr-javascript:latest

ENV PKG_CONFIG_PATH /usr/local/lib/pkgconfig:$PKG_CONFIG_PATH

###################################################
# install clang 3.5 for formatting
###################################################
RUN cd /tmp/ \
    && . /etc/profile \
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
    && . /etc/profile \
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
    && . /etc/profile \
    && git clone https://github.com/gabime/spdlog.git \
    && cd spdlog \
    && git checkout 799ba2a57bb16b921099bd9ab64a513e4ebe4217 \
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
    && . /etc/profile \
    && git clone https://github.com/zaphoyd/websocketpp.git \
    && cd websocketpp \
    && git checkout 0.8.1 \
    && mkdir build \
    && cd build \
    && cmake .. \
    && make install -j"$(nproc)" \
    && cd /opt/ \
    && rm -rf websocketpp

###################################################
# install gcovr for code coverage reports
###################################################
RUN dnf update -y \
    && . /etc/profile \
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
# install boost
###################################################
RUN dnf update -y \
    && dnf install -y \
    boost \
    boost-devel \
    && dnf clean all

###################################################
# install googletest & googlemock
###################################################

RUN cd /opt \
    && . /etc/profile \
    && git clone https://github.com/google/googletest.git \
    && cd googletest \
    && git checkout ddb8012e \
    && mkdir build \
    && cd build \
    && cmake -DCMAKE_POSITION_INDEPENDENT_CODE=ON .. \
    && make install -j"$(nproc)" \
    && cd /opt/ \
    && rm -rf googletest

###################################################
# install psmisc tools (includes pstree)
###################################################
RUN dnf update -y \
    && . /etc/profile \
    && dnf install -y \
    psmisc \
    && dnf clean all

RUN chmod -R a+rw /opt \
    && chown -R 1000 /usr/local

###################################################
# install flatbuffers
###################################################

RUN export FLATBUFFERS_VERSION=v1.10.0 \
        && . /etc/profile \
        && cd /tmp \
        && git clone https://github.com/google/flatbuffers.git \
        && cd flatbuffers \
        && git checkout $FLATBUFFERS_VERSION \
        && mkdir build \
        && cd build \
        && cmake .. -DFLATBUFFERS_BUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Release \
        && make install -j"$(nproc)" \
        && rm -rf /tmp/flatbuffers

###################################################
# install smrf
###################################################

RUN export SMRF_VERSION=0.3.3 \
    && . /etc/profile \
    && cd /opt \
    && git clone https://github.com/bmwcarit/smrf.git \
    && cd smrf \
    && git checkout $SMRF_VERSION \
    && mkdir build \
    && cd build \
    && cmake -DBUILD_TESTS=Off .. \
    && make install -j"$(nproc)" \
    && cd /opt \
    && rm -rf smrf

###################################################
# install MoCOCrW
###################################################

RUN export MoCOCrW_VERSION=openssl1.1 \
    && . /etc/profile \
    && cd /opt \
    && git clone https://github.com/bmwcarit/MoCOCrW.git \
    && cd MoCOCrW \
    && git checkout $MoCOCrW_VERSION \
    && mkdir build \
    && cd build \
    && cmake -DBUILD_TESTING=Off .. \
    && make install -j"$(nproc)" \
    && cd /opt \
    && rm -rf MoCOCrW

###################################################
# install valgrind
###################################################

RUN export VALGRIND_VERSION=3.13.0 \
        && . /etc/profile \
        && cd /tmp \
        && wget https://sourceware.org/pub/valgrind/valgrind-$VALGRIND_VERSION.tar.bz2 \
        && tar xf valgrind-$VALGRIND_VERSION.tar.bz2 \
        && cd valgrind-$VALGRIND_VERSION \
        && ./configure \
        && make install -j"$(nproc)" \
        && rm -rf /tmp/valgrind-$VALGRIND_VERSION /tmp/valgrind-$VALGRIND_VERSION.tar.bz2

###################################################
# Copy build scripts
###################################################
COPY scripts /data/scripts

###################################################
# setup build environment
###################################################
RUN mkdir -p /home/joynr/
RUN mkdir /home/joynr/build
RUN date -R > /data/timestamp
RUN chmod -R a+rwx /usr/local
RUN echo "/usr/local/lib64" > /etc/ld.so.conf.d/usr-local-lib64.conf && ldconfig
