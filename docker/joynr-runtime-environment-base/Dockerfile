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
COPY scripts/docker/setup-proxy.sh /data/scripts/setup-proxy.sh
COPY scripts/docker/setup-extra-certs.sh /data/scripts/setup-extra-certs.sh

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
	wget \
	procps \
	java-1.8.0-openjdk-headless \
	java-11-openjdk \
	java-11-openjdk-devel \
	&& dnf clean all

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
