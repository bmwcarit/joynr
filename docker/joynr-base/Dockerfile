FROM fedora:36

LABEL com.jfrog.artifactory.retention.maxCount="25"

# set this date to cause all images to be updated
ENV REFRESHED_AT 2022-05-02-15-00

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
RUN mkdir -p /home/joynr/
RUN mkdir /home/joynr/build

###################################################
# copy scripts and set start command
###################################################
COPY scripts/docker /data/scripts
RUN chmod 777 -R /data/scripts/

###################################################
# Setup dnf.conf
###################################################
RUN /data/scripts/setup-proxy.sh

###################################################
# set timezone
###################################################
RUN rm /etc/localtime \
    && ln -s /usr/share/zoneinfo/Europe/Berlin /etc/localtime

###################################################
# install base packages
###################################################
RUN dnf update -y \
    && dnf install -y \
    autoconf \
    automake \
    ccache \
    cmake \
    corkscrew \
    expat-devel \
    file \
    gcc-c++ \
    gdb \
    git \
    glibc.i686 \
    glibc-devel.i686 \
    icecream \
    iputils \
    libasan \
    libasan-static \
    libcurl \
    libcurl-devel \
    libstdc++.i686 \
    libtool \
    libtsan \
    libtsan-static \
    libubsan \
    libubsan-static \
    libuuid-devel \
    libX11-devel \
    libxcb \
    libxcb-devel \
    libXi-devel \
    libXrender-devel \
    mesa-libGLU.i686 \
    ncurses-devel.i686 \
    net-tools \
    openssl \
    openssl1.1 \
    openssl1.1-devel \
    patch \
    perl-version \
    python \
    strace \
    tar \
    unzip \
    wget \
    which \
    xcb-util \
    xcb-util-*-devel \
    xcb-util-devel \
    zlib-devel \
    zlib-devel.i686 \
    zlib.i686 \
# when changing this, also adjust the joynr-java-8 docker image
# so that the proper package is uninstalled
    java-11-openjdk \
    java-11-openjdk-devel \
    xz \
    maven \
    hostname.x86_64 \
    clang \
    libcxx-devel \
    && dnf groupinstall -y 'Development Tools' \
    && dnf clean all

###################################################
# configure Java 11 as default
# otherwise we would have to remove several
# packages depending on java8 in order to be able
# to remove java8 packages
###################################################
RUN alternatives --set java /usr/lib/jvm/java-11-openjdk*.x86_64/bin/java \
    && alternatives --set javac /usr/lib/jvm/java-11-openjdk*.x86_64/bin/javac \
    && alternatives --set jre_openjdk /usr/lib/jvm/java-11-openjdk*.x86_64 \
    && alternatives --set java_sdk_openjdk /usr/lib/jvm/java-11-openjdk*.x86_64

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
# Install postgresql
###################################################
RUN . /etc/profile \
    && dnf install -y postgresql-server postgresql-contrib \
    && echo "gcd" > /opt/gcd_password \
    && chown postgres /opt/gcd_password \
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
    && make WITH_WEBSOCKETS=no DOCDIRS= prefix=/usr install -j4

###################################################
# copy mosquitto.conf
###################################################
COPY mosquitto.conf /etc/mosquitto/mosquitto.conf

###################################################
# install android sdk
###################################################
RUN dnf update -y \
    && dnf install -y \
    glibc-devel.i686 \
    mesa-libGLU.i686 \
    ncurses-devel.i686 \
    zlib-devel.i686 \
    && dnf clean all

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
# set this date to cause android to be updated
ENV REFRESHED_ANDROID_AT 2020-07-08
RUN /data/scripts/setup-android.sh

###################################################
# install node.js
###################################################
# nvm environment variables
ENV NVM_DIR /usr/local/nvm

ENV NODE_V8 8.16.2
ENV NODE_V12 12.22.2

# install nvm
RUN . /etc/profile \
    && mkdir -p $NVM_DIR \
    && curl --silent -o- https://raw.githubusercontent.com/creationix/nvm/v0.39.1/install.sh | bash

# install node and npm
# having the nvm directory writable makes it possible to use nvm to change node versions manually
# nvm uses curl internally with '-q' option suppressing evaluation of '.curlrc' hence
# if a proxy is set it is required to wrap curl to explicitly set a config file because
# nvm does not provide an option for this.
RUN . /etc/profile \
    && if [ -n "$PROXY_HOST" ]; then alias curl="/usr/bin/curl -K /etc/.curlrc"; fi \
    && source $NVM_DIR/nvm.sh \
    && nvm install $NODE_V12 \
    && nvm install $NODE_V8 \
    && nvm alias default $NODE_V8 \
    && nvm use default \
    && chmod -R a+rwx $NVM_DIR

# add node and npm to path
# (node will be available then without sourcing $NVM_DIR/nvm.sh)
ENV PATH $NVM_DIR/versions/node/v$NODE_V8/bin:$PATH

###################################################
# provide environment for joynr C++
###################################################

ENV PKG_CONFIG_PATH /usr/local/lib/pkgconfig:$PKG_CONFIG_PATH

###################################################
# enable gold linker
###################################################
RUN  update-alternatives  --set ld /usr/bin/ld.gold

###################################################
# install dnf packages for joynr C++
###################################################
RUN dnf update -y \
    && . /etc/profile \
    && dnf install -y \
    rpm-build \
    python-pip \
    lcov \
    boost \
    boost-devel \
    ninja-build \
    psmisc \
    && dnf clean all \
    && pip install gcovr

RUN chmod -R a+rw /opt \
    && chown -R 1000 /usr/local

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
# install spdlog
###################################################

RUN cd /opt \
    && . /etc/profile \
    && git clone https://github.com/gabime/spdlog.git \
    && cd spdlog \
    && git checkout v1.4.2 \
    && mkdir build \
    && cd build \
    && cmake -DSPDLOG_BUILD_BENCH=OFF -DSPDLOG_BUILD_TESTS=OFF .. \
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

# DLT installs itself in /usr/local/lib64
ENV PKG_CONFIG_PATH $PKG_CONFIG_PATH:/usr/local/lib64/pkgconfig

###################################################
# install flatbuffers
###################################################

RUN export FLATBUFFERS_VERSION=v1.12.1 \
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
# install valgrind
###################################################

RUN export VALGRIND_VERSION=3.19.0 \
        && . /etc/profile \
        && cd /tmp \
        && wget https://sourceware.org/pub/valgrind/valgrind-$VALGRIND_VERSION.tar.bz2 \
        && tar xf valgrind-$VALGRIND_VERSION.tar.bz2 \
        && cd valgrind-$VALGRIND_VERSION \
        && ./configure \
        && make install -j"$(nproc)" \
        && rm -rf /tmp/valgrind-$VALGRIND_VERSION /tmp/valgrind-$VALGRIND_VERSION.tar.bz2

###################################################
# install gcovr for code coverage reports
###################################################
RUN . /etc/profile \
    && dnf update -y \
    && dnf install -y \
    lcov \
    python-pip \
    && dnf clean all \
    && pip install gcovr

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
RUN chmod -R a+rwx /usr/local
RUN echo "/usr/local/lib64" > /etc/ld.so.conf.d/usr-local-lib64.conf && ldconfig

###################################################
# add Tini - "A tiny but valid init for containers"
###################################################
ENV TINI_VERSION v0.13.1
ADD https://github.com/krallin/tini/releases/download/${TINI_VERSION}/tini /tini
RUN chmod +x /tini

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

ENTRYPOINT ["/tini", "-g", "--", "/data/scripts/boot2user.sh"]
