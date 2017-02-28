FROM fedora:24

# set this date to cause all images to be updated
ENV REFRESHED_AT 2017-02-13

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
	dbus-x11 \
	expat-devel \
	file \
	gcc-c++ \
	gdb \
	git \
	glibc.i686 \
	icecream \
	libcurl \
	libcurl-devel \
	libstdc++.i686 \
	libtool \
	libxcb \
	libxcb-devel \
	libX11-devel \
	libXi-devel \
	libXrender-devel \
	openssl \
	openssl-devel \
	patch \
	perl-version \
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
# build and install mosquitto 1.4.7 mqtt broker
# do not build documentation
# run mosquitto as user "joynr"
# do not limit message queue
# use settings in /etc/mosquitto/mosquitto.conf
###################################################
RUN dnf update -y \
    && dnf install -y mosquitto \
    mosquitto-devel \
    && sed -i "s/#max_queued_messages 100/max_queued_messages 0/" /etc/mosquitto/mosquitto.conf \
    && sed -i "s/#user mosquitto/user joynr/" /etc/mosquitto/mosquitto.conf \
    && sed -i "s/#listener/listener 1883\n\nlistener 9001\nprotocol websockets/" /etc/mosquitto/mosquitto.conf \
    && dnf clean all

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


###################################################
# copy scripts and set start command
###################################################
COPY scripts /data/scripts

###################################################
# Generate certificates for system tests
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
    asadmin --user admin stop-domain

RUN chmod -R 777 /opt/payara41

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
	&& curl -o gosu -sSL "https://github.com/tianon/gosu/releases/download/${GOSU_VERSION}/gosu-amd64" \
	&& mv gosu /usr/local/bin/gosu \
	&& chmod 755 /usr/local/bin/gosu

COPY scripts/boot2user.sh /data/scripts/boot2user.sh

ENTRYPOINT ["/tini", "-g", "--", "/data/scripts/boot2user.sh"]
