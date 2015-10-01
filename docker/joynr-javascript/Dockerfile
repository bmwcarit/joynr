FROM joynr-android:latest

###################################################
# install node.js
###################################################
RUN yum install -y \
	nodejs \
	npm \
	&& yum clean all

###################################################
# install libs needed to run phantomjs
###################################################
RUN yum install -y \
	freetype-devel \
	fontconfig-devel \
	&& yum clean all

###################################################
# deploy the build scripts
###################################################
RUN mkdir /data/scripts/build
COPY scripts/build/* /data/scripts/build/
RUN chmod 777 -R /data/scripts/build/
