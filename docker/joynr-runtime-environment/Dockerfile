# Following must be copied to the docker build context before the docker images is built
# - {joynr}/javascript/libjoynr-js as files/libjoynr-js
# - {joynr}/tests/test-base as files/test-base
# - {joynr}/java/messaging/bounceproxy/single-bounceproxy/target/single-bounceproxy.war as files/single-bounceproxy.war
#
# Following files must be copied from {joynr}/tests/performance-test/ to files/performance-test/
# - src/
# - package.json
# - run-performance-test.sh

FROM joynr-runtime-environment-base:latest

###################################################
# install the c++ executables which are provided
# as rpm files. The rpm files are created by using
# following shell script:
# docker/joynr-cpp-base/scripts/build/cpp-build-rpm-package.sh
# The created rpm files must be copied to the docker
# build context beforehand.
###################################################
ARG JOYNR_RPM_NAME
ADD ${JOYNR_RPM_NAME} /tmp/joynr.rpm

ARG JOYNR_TEST_RPM_NAME
ADD ${JOYNR_TEST_RPM_NAME} /tmp/joynr-test.rpm

ARG SMRF_RPM_NAME
ADD ${SMRF_RPM_NAME} /tmp/smrf.rpm

ARG MOCOCRW_RPM_NAME
ADD ${MOCOCRW_RPM_NAME} /tmp/MoCOCrW.rpm

# This line is necessary to ensure that the following dnf
# call can access the internet.
RUN . /etc/profile && curl www.google.de > /dev/null

RUN dnf install -y \
	/tmp/MoCOCrW.rpm \
	/tmp/smrf.rpm \
	/tmp/joynr.rpm \
	/tmp/joynr-test.rpm \
	&& rm /tmp/MoCOCrW.rpm \
	&& rm /tmp/smrf.rpm \
	&& rm /tmp/joynr.rpm \
	&& rm /tmp/joynr-test.rpm

###################################################
# install js dependencies
###################################################
RUN . /etc/profile && npm install -g bluebird@3.1.1 jasmine-node

###################################################
# install js joynr as a global package
###################################################
COPY files/libjoynr-js /tmp/libjoynr-js

RUN . /etc/profile && cd /tmp/libjoynr-js/target/node-classes \
	&& npm install -g \
	&& rm -rf /tmp/libjoynr-js

###################################################
# install js test-base as a global package
###################################################
COPY files/test-base /tmp/test-base

RUN . /etc/profile && cd /tmp/test-base/ \
	&& npm config set http-proxy ${http_proxy} \
	&& npm config set https-proxy ${https_proxy} \
	&& npm install -g \
	&& rm -rf /tmp/test-base

###################################################
# install the performance-tests
###################################################
COPY files/performance-test/src /opt/joynr/performance-test/src
COPY files/performance-test/package.json /opt/joynr/performance-test/package.json
COPY files/performance-test/run-performance-tests.sh /opt/joynr/performance-test/run-performance-tests.sh
COPY files/performance-test/performance-test-consumer.jar /opt/joynr/performance-test/performance-test-consumer.jar
COPY files/performance-test/performance-test-provider.jar /opt/joynr/performance-test/performance-test-provider.jar

###################################################
# install the mosquitto configuration
###################################################
COPY files/mosquitto.conf /etc/mosquitto/mosquitto.conf

###################################################
# install the backend servlets
###################################################

ADD files/single-bounceproxy.war /usr/local/jetty/webapps/bounceproxy.war

# Create an environment variable which is used by
# npm and points to the installation folder of the
# global dependencies.
ENV NODE_PATH /usr/lib/node_modules/
