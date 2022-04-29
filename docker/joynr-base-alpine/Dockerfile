FROM alpine:latest

LABEL com.jfrog.artifactory.retention.maxCount="25"

# set this date to cause all images to be updated
ENV REFRESHED_AT 2022-04-25-09-00

###################################################
# create data directories and volumes
###################################################
WORKDIR /
RUN mkdir /data

VOLUME /data/install
VOLUME /data/src
VOLUME /data/build

ENV BUILD_DIR /data/build
ENV SRC_DIR /data/src
ENV INSTALL_DIR /data/install
ENV CURL_HOME /etc

###################################################
# setup build environment
###################################################
RUN mkdir -p /home/joynr/build
RUN mkdir -p /data/scripts
RUN mkdir -p /usr/local/include

###################################################
# copy scripts and set start command
###################################################
COPY scripts/docker /data/scripts

###################################################
# bash is required for our scripts unless they are
# rewritten to be compatible to busybox stripped
# down shell.
# We need to install bash ahead here.
###################################################
RUN apk update \
    && apk add \
    bash

###################################################
# Setup proxy settings
###################################################
RUN /data/scripts/setup-proxy.sh

###################################################
# install base packages
###################################################
RUN apk update \
    && apk add \
    alpine-conf \
    autoconf \
    automake \
    binutils \
    binutils-gold \
    boost \
    boost-dev \
    ccache \
    clang \
    clang-extra-tools \
    cmake \
    coreutils \
    curl \
    doxygen \
    file \
    findutils  \
    gcc \
    g++ \
    gcompat \
    gdb \
    git \
    grep \
    iproute2 \
    iputils \
    less \
    libcurl \
    libtool \
    libuuid \
    libwebsockets-dev \
    libx11-dev \
    libxcb-dev \
    libxi-dev \
    libxml2-dev \
    libxrender-dev \
    libxslt-dev \
    lsof \
    make \
    maven \
    musl-locales \
    ncurses-dev \
    net-tools \
    nodejs \
    npm \
    openjdk11 \
    openssl \
    openssl-dev \
    patch \
    postgresql \
    procps \
    psmisc \
    python3 \
    python3-dev \
    py3-pip \
    samurai \
    strace \
    tar \
    tini \
    util-linux  \
    unzip \
    valgrind \
    vim \
    wget \
    which \
    xcb-util \
    xcb-util-keysyms-dev \
    xcb-util-xrm-dev \
    xcb-util-image-dev \
    xcb-util-renderutil-dev \
    xcb-util-cursor-dev \
    xcb-util-wm-dev \
    xcb-util-dev \
    xz \
    zlib \
    zlib-dev

###################################################
# set timezone
###################################################
RUN setup-timezone -z Europe/Berlin

###################################################
# setup extra certs
###################################################
RUN /data/scripts/setup-extra-certs.sh

###################################################
# add git config
###################################################
RUN git config --global user.email "dummy@dummy.com" \
    && git config --global user.name "Dummy"

###################################################
# Generate certificates
###################################################
COPY openssl.conf /tmp/openssl.cnf
RUN mkdir -p /data/ssl-data \
    && /data/scripts/gen-certificates.sh --configfile /tmp/openssl.cnf --destdir /data/ssl-data

###################################################
# Configure postgresql
###################################################
RUN . /etc/profile \
    && echo "gcd" > /opt/gcd_password \
    && chown postgres /opt/gcd_password

RUN . /etc/profile \
    && mkdir /run/postgresql \
    && chown postgres:postgres /run/postgresql \
    && su - postgres /data/scripts/configure-db.sh

###################################################
# Install payara
###################################################
ENV PATH ${PATH}:/opt/payara5/glassfish/bin

RUN wget --quiet -O /opt/payara-5.2021.7.zip https://s3-eu-west-1.amazonaws.com/payara.fish/Payara+Downloads/5.2021.7/payara-5.2021.7.zip && \
    unzip -qq /opt/payara-5.2021.7.zip -d /opt && \
    rm /opt/payara-5.2021.7.zip

RUN asadmin --user admin start-domain && \
    asadmin --user admin create-managed-scheduled-executor-service --corepoolsize=100 concurrent/joynrMessagingScheduledExecutor && \
    asadmin --user admin set-log-levels io.joynr.messaging=FINE && \
    asadmin --user admin set-log-levels io.joynr.dispatching=FINE && \
    asadmin --user admin set-log-levels io.joynr.discovery=FINEST && \
    asadmin --user admin set-log-levels io.joynr.capabilities=FINEST && \
    asadmin --user admin set-log-levels io.joynr.arbitration=FINEST && \
    asadmin --user admin set-log-levels io.joynr.proxy=FINE && \
    asadmin --user admin set-log-levels io.joynr.jeeintegration=FINEST && \
    asadmin --user admin set-log-attributes com.sun.enterprise.server.logging.GFFileHandler.multiLineMode=false && \
    asadmin --user admin stop-domain --kill=true

RUN chmod -R 777 /opt/payara5

###################################################################################
# build and install mosquitto 1.6.12 mqtt broker
# with additional fixes and joynr API extension
# do not build documentation
# use default settings
# Extra Git commits from upstream (to be removed when updating to 1.6.13 or later):
# 298d8494 Fix send quota being incorrecly reset on reconnect.
# 3806296c Ld symbol of the mosquitto_property_copy_all has global bind now.
# 7804c3f0 Note that 1024 "limit" is from operating systems, not Mosquitto.
# 938e17a3 Fix incorrect authentication-method property type in mosquitto_sub man.
# 0bdf630c (tag: v1.6.12) Merge branch 'fixes'
###################################################################################
COPY 0001-Introduce-mosquitto_connect_bind_async_v5-API.patch /tmp/0001-Introduce-mosquitto_connect_bind_async_v5-API.patch
COPY 0002-Added-reconnect_session_expiry_interval-to-struct-mo.patch /tmp/0002-Added-reconnect_session_expiry_interval-to-struct-mo.patch
COPY 0003-Introduced-option-MOSQ_OPT_RECONNECT_SESSION_EXPIRY_.patch /tmp/0003-Introduced-option-MOSQ_OPT_RECONNECT_SESSION_EXPIRY_.patch
COPY 0004-mosquitto_reconnect-supports-MQTTv5-session-expiry.patch /tmp/0004-mosquitto_reconnect-supports-MQTTv5-session-expiry.patch
RUN . /etc/profile \
    && cd /opt \
    && git clone https://github.com/eclipse/mosquitto \
    && cd mosquitto \
    && git checkout 298d8494 \
    && git am -k /tmp/0001-Introduce-mosquitto_connect_bind_async_v5-API.patch \
    && git am -k /tmp/0002-Added-reconnect_session_expiry_interval-to-struct-mo.patch \
    && git am -k /tmp/0003-Introduced-option-MOSQ_OPT_RECONNECT_SESSION_EXPIRY_.patch \
    && git am -k /tmp/0004-mosquitto_reconnect-supports-MQTTv5-session-expiry.patch \
    && make WITH_WEBSOCKETS=yes DOCDIRS= prefix=/usr install -j4

###################################################
# copy mosquitto.conf
###################################################
COPY mosquitto.conf /etc/mosquitto/mosquitto.conf

###################################################
# install android sdk
###################################################

ENV ANDROID_SDK_FILENAME commandlinetools-linux-7583922_latest.zip
ENV ANDROID_SDK_URL https://dl.google.com/android/repository/${ANDROID_SDK_FILENAME}
ENV ANDROID_API_LEVEL android-28
ENV ANDROID_BUILD_TOOLS_VERSION 29.0.3
ENV ANDROID_HOME /opt/android-sdk-linux
ENV PATH ${PATH}:${ANDROID_HOME}/tools:${ANDROID_HOME}/platform-tools
RUN cd /opt \
    && mkdir -p ${ANDROID_HOME} \
    && cd ${ANDROID_HOME} \
    && wget -q ${ANDROID_SDK_URL} \
    && unzip ${ANDROID_SDK_FILENAME} \
    && rm ${ANDROID_SDK_FILENAME} \
    && mv cmdline-tools tmp-cmdline-tools \
    && mkdir -p ${ANDROID_HOME}/cmdline-tools/latest \
    && mv tmp-cmdline-tools/* ${ANDROID_HOME}/cmdline-tools/latest \
    && rm -fr tmp-cmdline-tools

RUN chown -R root:root ${ANDROID_HOME} \
    && chmod -R 755 ${ANDROID_HOME}
RUN /data/scripts/setup-android.sh

###################################################
# provide environment for joynr C++
###################################################

ENV PKG_CONFIG_PATH /usr/local/lib/pkgconfig:$PKG_CONFIG_PATH

###################################################
# install gcovr
###################################################
RUN . /etc/profile \
    && pip install gcovr

RUN chmod -R a+rw /opt \
    && chown -R 1000 /usr/local

###################################################
# install clang-format-3.5
# This can be taken from Alpine 3.1 where
# llvm 3.5.0 was current
###################################################
RUN . /etc/profile \
    && mkdir -p /tmp/clang-format \
    && cd /tmp/clang-format \
    && wget http://dl-cdn.alpinelinux.org/alpine/v3.1/main/x86_64/llvm-libs-3.5.0-r0.apk \
    && tar xvf llvm-libs-3.5.0-r0.apk \
    && cp usr/lib/libLLVM-3.5.so /usr/lib \
    && chown root /usr/lib/libLLVM-3.5.so \
    && chgrp root /usr/lib/libLLVM-3.5.so \
    && chmod  755 /usr/lib/libLLVM-3.5.so \
    && wget http://dl-cdn.alpinelinux.org/alpine/v3.1/main/x86_64/clang-3.5.0-r0.apk \
    && tar xvf clang-3.5.0-r0.apk \
    && cp usr/bin/clang-format /usr/bin/clang-format-3.5 \
    && chown root /usr/bin/clang-format-3.5 \
    && chgrp root /usr/bin/clang-format-3.5 \
    && chmod  755 /usr/bin/clang-format-3.5 \
    && mv /usr/bin/clang-format /usr/bin/clang-format-12.0.1 \
    && ln -s /usr/bin/clang-format-3.5 /usr/bin/clang-format \
    && cd /tmp \
    && rm -fr /tmp/clang-format

###################################################
# install spdlog
###################################################
RUN cd /opt \
    && . /etc/profile \
    && git clone https://github.com/gabime/spdlog.git \
    && cd spdlog \
    && git checkout v1.4.2 \
    && mkdir build \
    && cd build \
    && cmake -DSPDLOG_BUILD_BENCH=OFF .. \
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
    && git checkout 0.8.2 \
    && mkdir build \
    && cd build \
    && cmake .. \
    && make install -j"$(nproc)" \
    && cd /opt/ \
    && rm -rf websocketpp

###################################################
# install DLT
###################################################

# create fake directory for DLT so it can be installed
# Alpine musl seems to work differently
RUN mkdir -p /etc/ld.so.conf.d

RUN cd /opt \
    && . /etc/profile \
    && git clone https://github.com/GENIVI/dlt-daemon \
    && cd dlt-daemon \
    && git checkout v2.18.8 \
    && sed -i 's/libdir=${exec_prefix}\/lib/libdir=@CMAKE_INSTALL_FULL_LIBDIR@/' automotive-dlt.pc.in \
    && sed -i 's/includedir=${exec_prefix}\/include/includedir=@CMAKE_INSTALL_FULL_INCLUDEDIR@/' automotive-dlt.pc.in \
    && sed -i 's/-Werror/-Wno-error/' src/console/logstorage/CMakeLists.txt \
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
RUN export SMRF_VERSION=0.3.4 \
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
RUN export MoCOCrW_VERSION=c5609ccc1f3da552b4354b747bdc445e4ecfc7de \
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
# install gcovr for code coverage reports
###################################################
RUN . /etc/profile \
    && pip install gcovr

###################################################
# install lcov
###################################################
RUN cd /opt \
    && . /etc/profile \
    && git clone https://github.com/linux-test-project/lcov.git \
    && cd lcov \
    && make install PREFIX=/usr \
    && cd /opt \
    && rm -rf lcov

###################################################
# install rapidjson
###################################################
RUN cd /opt \
    && . /etc/profile \
    && git clone https://github.com/miloyip/rapidjson.git rapidjson \
    && cd rapidjson \
    && git checkout v1.1.0 \
    && mkdir build \
    && cd build \
    && cmake -DRAPIDJSON_BUILD_DOC=OFF \
    -DRAPIDJSON_BUILD_EXAMPLES=OFF \
    -DRAPIDJSON_BUILD_TESTS=OFF \
    -DRAPIDJSON_BUILD_THIRDPARTY_GTEST=OFF .. \
    && make install -j"$(nproc)" \
    && cd /opt \
    && rm -rf rapidjson

###################################################
# install muesli
###################################################
RUN cd /opt \
    && . /etc/profile \
    && git clone https://github.com/bmwcarit/muesli.git \
    && cd muesli \
    && git checkout 1.0.2 \
    && mkdir build \
    && cd build \
    && cmake -DBUILD_MUESLI_TESTS=Off -DUSE_PLATFORM_RAPIDJSON=On .. \
    && make install -j"$(nproc)" \
    && cd /opt \
    && rm -rf muesli

###################################################
# setup build environment
###################################################
RUN date -R > /data/timestamp
RUN chmod -R a+rwx /usr/local
RUN ldconfig /usr/lib /usr/local/lib

###################################################
# set login user joynr
###################################################
ENV GOSU_VERSION=1.3
RUN cd /tmp \
    && . /etc/profile \
    && curl -o gosu -sSL "https://github.com/tianon/gosu/releases/download/${GOSU_VERSION}/gosu-amd64" \
    && mv gosu /usr/local/bin/gosu \
    && chown root /usr/local/bin/gosu \
    && chmod 4755 /usr/local/bin/gosu

ENTRYPOINT ["/sbin/tini", "-g", "--", "/data/scripts/boot2user.sh"]
