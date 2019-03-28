FROM fedora:27

###################################################
# create data directories and volumes
###################################################
WORKDIR /
RUN mkdir /data

ARG PROXY_HOST_BUILD_ARG=""
ARG PROXY_PORT_BUILD_ARG=""

ENV PROXY_HOST=$PROXY_HOST_BUILD_ARG
ENV PROXY_PORT=$PROXY_PORT_BUILD_ARG

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
# install base packages
###################################################
# procps is installed because of the pkill command
# which is required by the run-performance-test script
RUN dnf update -y \
	&& dnf install -y \
	tar \
	nodejs \
	npm \
	wget \
	procps \
	java-1.8.0-openjdk-headless \
	&& dnf clean all

###################################################
# install jetty
###################################################
RUN . /etc/profile \
	&& wget -O /usr/local/jetty.tar.gz http://repo1.maven.org/maven2/org/eclipse/jetty/jetty-distribution/9.2.13.v20150730/jetty-distribution-9.2.13.v20150730.tar.gz \
	&& cd /usr/local \
	&& mkdir jetty \
	&& tar xzf jetty.tar.gz --strip-components=1 -C /usr/local/jetty \
	&& rm /usr/local/jetty.tar.gz

ENV BACKEND_HOST localhost
ENV JAVA_OPTS \
	-Djoynr.servlet.hostpath=http://localhost:8080 \
	-Djoynr.messaging.channelurldirectoryurl=http://localhost:8080/discovery/channels/discoverydirectory_channelid/ \
	-Djoynr.messaging.bounceproxyurl=http://localhost:8080/bounceproxy \
	-Djoynr.messaging.capabilitiesdirectoryurl=http://localhost:8080/discovery/channels/discoverydirectory_channelid/
