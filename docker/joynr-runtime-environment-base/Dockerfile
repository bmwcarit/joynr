FROM fedora:23

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
	procps \
	java-1.8.0-openjdk-headless \
	&& dnf clean all

###################################################
# install jetty
###################################################
ADD http://repo1.maven.org/maven2/org/eclipse/jetty/jetty-distribution/9.2.13.v20150730/jetty-distribution-9.2.13.v20150730.tar.gz /usr/local/jetty.tar.gz
RUN cd /usr/local \
	&& mkdir jetty \
	&& tar xzf jetty.tar.gz --strip-components=1 -C /usr/local/jetty

ENV BACKEND_HOST localhost
ENV JAVA_OPTS \
	-Djoynr.servlet.hostpath=http://localhost:8080 \
	-Djoynr.messaging.channelurldirectoryurl=http://localhost:8080/discovery/channels/discoverydirectory_channelid/ \
	-Djoynr.messaging.bounceproxyurl=http://localhost:8080/bounceproxy \
	-Djoynr.messaging.capabilitiesdirectoryurl=http://localhost:8080/discovery/channels/discoverydirectory_channelid/
