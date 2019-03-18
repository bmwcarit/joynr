FROM fedora:27

# set this date to cause all images to be updated
ENV REFRESHED_AT 2019-05-23-09-00

###################################################
# create data directories and volumes
###################################################
WORKDIR /
RUN mkdir /data

VOLUME /data/install
VOLUME /data/src
VOLUME /data/build

ARG PROXY_HOST_BUILD_ARG=""
ARG PROXY_PORT_BUILD_ARG=""

ENV PROXY_HOST=$PROXY_HOST_BUILD_ARG
ENV PROXY_PORT=$PROXY_PORT_BUILD_ARG

ENV BUILD_DIR /data/build
ENV SRC_DIR /data/src
ENV INSTALL_DIR /data/install
ENV CURL_HOME /etc

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
    libxcb \
    libxcb-devel \
    libX11-devel \
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
    xcb-util-devel \
    xcb-util-*-devel \
    zlib.i686 \
    java-1.8.0-openjdk \
    xz \
    maven \
    hostname.x86_64 \
    clang \
    libcxx-devel \
    && dnf groupinstall -y 'Development Tools' \
    && dnf clean all

###################################################
# Generate certificates
###################################################
COPY openssl.conf /tmp/openssl.cnf
RUN mkdir -p /data/ssl-data \
    && /data/scripts/gen-certificates.sh --configfile /tmp/openssl.cnf --destdir /data/ssl-data

###################################################
# Install payara
###################################################
ENV PATH ${PATH}:/opt/payara41/glassfish/bin

RUN wget --quiet -O /opt/payara-4.1.1.164.zip https://s3-eu-west-1.amazonaws.com/payara.fish/Payara+Downloads/Payara+4.1.1.164/payara-4.1.1.164.zip && \
    unzip -qq /opt/payara-4.1.1.164.zip -d /opt && \
    rm /opt/payara-4.1.1.164.zip

RUN asadmin start-domain && \
    asadmin --user admin create-managed-scheduled-executor-service --corepoolsize=100 concurrent/joynrMessagingScheduledExecutor && \
    asadmin --user admin create-jdbc-connection-pool --datasourceclassname org.apache.derby.jdbc.ClientDataSource --restype javax.sql.XADataSource --property portNumber=1527:password=APP:user=APP:serverName=localhost:databaseName=joynr-discovery-directory:connectionAttributes=\;create\\=true JoynrPool && \
    asadmin --user admin create-jdbc-resource --connectionpoolid JoynrPool joynr/DiscoveryDirectoryDS && \
    asadmin --user admin create-jdbc-resource --connectionpoolid JoynrPool joynr/DomainAccessControllerDS && \
    asadmin --user admin set-log-levels io.joynr.messaging=FINE && \
    asadmin --user admin set-log-levels io.joynr.dispatching=FINE && \
    asadmin --user admin set-log-levels io.joynr.jeeintegration=FINE && \
    asadmin --user admin set configs.config.server-config.network-config.network-listeners.network-listener.http-listener-1.enabled=false && \
    asadmin --user admin stop-domain --kill=true

RUN chmod -R 777 /opt/payara41

###################################################
# install mosquitto mqtt broker
###################################################
RUN dnf update -y \
    && dnf install -y mosquitto \
    mosquitto-devel \
    && dnf clean all
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
    && chmod 755 /usr/local/bin/gosu

COPY scripts/boot2user.sh /data/scripts/boot2user.sh

ENTRYPOINT ["/tini", "-g", "--", "/data/scripts/boot2user.sh"]
