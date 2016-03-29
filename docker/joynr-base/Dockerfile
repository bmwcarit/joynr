FROM fedora:23

# set this date to cause all images to be updated
ENV REFRESHED_AT 2016-02-16

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
	&& dnf groupinstall -y 'Development Tools' \
	&& dnf clean all

###################################################
# build and install mosquitto 1.4.7 mqtt broker
# do not build documentation
# use default settings
# run mosquitto as user "joynr"
###################################################
RUN dnf update -y \
    && dnf install -y c-ares-devel libuuid-devel\
    && cd /opt \
    && git clone https://git.eclipse.org/r/mosquitto/org.eclipse.mosquitto.git \
    && cd org.eclipse.mosquitto \
    && git checkout v1.4.7 \
    && make DOCDIRS= install -j"$(nproc)" \
    && mv /etc/mosquitto/mosquitto.conf.example /etc/mosquitto/mosquitto.conf \
    && sed -i "s/#max_queued_messages 100/max_queued_messages 0/" /etc/mosquitto/mosquitto.conf \
    && echo "user joynr" >> /etc/mosquitto/mosquitto.conf \
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
# set login user joynr
###################################################
ENV GOSU_VERSION=1.3
RUN cd /tmp \
	&& curl -o gosu -sSL "https://github.com/tianon/gosu/releases/download/${GOSU_VERSION}/gosu-amd64" \
	&& mv gosu /usr/local/bin/gosu \
	&& chmod 755 /usr/local/bin/gosu

COPY scripts/boot2user.sh /data/scripts/boot2user.sh
ENTRYPOINT ["/data/scripts/boot2user.sh"]
