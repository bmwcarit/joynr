FROM fedora:21

# set this date to cause all images to be updated
ENV REFRESHED_AT 2015-04-10

###################################################
# set timezone
###################################################
RUN rm /etc/localtime \
	&& ln -s /usr/share/zoneinfo/Europe/Berlin /etc/localtime

###################################################
# install base packages
###################################################
RUN yum update -y \
	&& yum install -y \
	autoconf \
	automake \
	cmake \
	dbus-x11 \
	expat-devel \
	file \
	gcc-c++ \
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
	&& yum groupinstall -y 'Development Tools' \
	&& yum clean all

###################################################
# install Oracle JDK
###################################################
RUN cd /opt \
	&& wget --no-cookies --no-check-certificate --header "Cookie: gpw_e24=http%3A%2F%2Fwww.oracle.com%2F; oraclelicense=accept-securebackup-cookie" \
		"http://download.oracle.com/otn-pub/java/jdk/8u40-b26/jdk-8u40-linux-x64.tar.gz"

RUN cd /opt \
	&& tar xvf jdk-8u40-linux-x64.tar.gz \
	&& alternatives --install /usr/bin/java java /opt/jdk1.8.0_40/bin/java 18000400 \
	&& alternatives --install /usr/bin/javac javac /opt/jdk1.8.0_40/bin/javac 18000400 \
	&& alternatives --install /usr/bin/jar jar /opt/jdk1.8.0_40/bin/jar 18000400 \
	&& rm /opt/jdk-8u40-linux-x64.tar.gz

###################################################
# install Maven
###################################################
WORKDIR /opt

ENV MAVEN_VERSION=3.3.3
RUN wget http://mirror.serversupportforum.de/apache/maven/maven-3/${MAVEN_VERSION}/binaries/apache-maven-${MAVEN_VERSION}-bin.tar.gz \
	&& tar -xf apache-maven-${MAVEN_VERSION}-bin.tar.gz \
	&& rm apache-maven-${MAVEN_VERSION}-bin.tar.gz \
	&& mkdir -p /home/joynr/.m2

ENV PATH $PATH:/opt/apache-maven-${MAVEN_VERSION}/bin
ENV MAVEN_OPTS -Xmx2048M -XX:+CMSClassUnloadingEnabled
VOLUME /home/joynr/.m2

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
