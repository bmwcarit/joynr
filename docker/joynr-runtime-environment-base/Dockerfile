FROM fedora:36

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
COPY scripts/docker /data/scripts
RUN chmod 777 -R /data/scripts/

###################################################
# Setup dnf.conf
###################################################
RUN /data/scripts/setup-proxy.sh

###################################################
# setup extra certs for general use w/o java
###################################################
RUN /data/scripts/setup-extra-certs.sh

###################################################
# install base packages
###################################################
# procps is installed because of the pkill command
# which is required by the run-performance-test script
RUN dnf update -y \
	&& dnf install -y \
	java-11-openjdk \
	java-11-openjdk-devel \
	openssl \
	openssl1.1 \
	openssl1.1-devel \
	procps \
	tar \
	wget \
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
# setup extra certs for java
###################################################
RUN /data/scripts/setup-extra-certs.sh java
