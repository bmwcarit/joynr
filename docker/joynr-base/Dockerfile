FROM fedora:30

LABEL com.jfrog.artifactory.retention.maxCount="25"

# set this date to cause all images to be updated
ENV REFRESHED_AT 2021-09-20-08-00

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
COPY scripts /data/scripts

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
    icecream \
    iputils \
    libcurl \
    libcurl-devel \
    libstdc++.i686 \
    libtool \
    libuuid-devel \
    libwebsockets-devel \
    libX11-devel \
    libxcb \
    libxcb-devel \
    libXi-devel \
    libXrender-devel \
    net-tools \
    openssl \
    openssl-devel \
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
    asadmin --user admin create-jdbc-connection-pool --datasourceclassname org.apache.derby.jdbc.ClientDataSource --restype javax.sql.XADataSource --property portNumber=1527:password=APP:user=APP:serverName=localhost:databaseName=joynr-discovery-directory:connectionAttributes=\;create\\=true JoynrPool && \
    asadmin --user admin create-jdbc-resource --connectionpoolid JoynrPool joynr/DiscoveryDirectoryDS && \
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
    && chmod 4755 /usr/local/bin/gosu

COPY scripts/boot2user.sh /data/scripts/boot2user.sh

ENTRYPOINT ["/tini", "-g", "--", "/data/scripts/boot2user.sh"]
