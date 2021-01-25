FROM fedora:30

LABEL com.jfrog.artifactory.retention.maxCount="25"

###################################################
# create data directories and volumes
###################################################
WORKDIR /
RUN mkdir /data

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
# setup extra certs
###################################################
RUN /data/scripts/setup-extra-certs.sh

###################################################
# add git config
###################################################
#RUN git config --global user.email "dummy@dummy.com" \
#    && git config --global user.name "Dummy"

###################################################
# install jetty
###################################################
RUN . /etc/profile \
	&& wget -O /usr/local/jetty.tar.gz https://repo1.maven.org/maven2/org/eclipse/jetty/jetty-distribution/9.2.13.v20150730/jetty-distribution-9.2.13.v20150730.tar.gz \
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
