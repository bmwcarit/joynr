FROM joynr-android:latest

###################################################
# install node.js
###################################################
RUN dnf install -y \
	nodejs \
	npm \
	&& dnf clean all

###################################################
# install libs needed to run phantomjs
###################################################
RUN dnf install -y \
	freetype-devel \
	fontconfig-devel \
	&& dnf clean all

###################################################
# deploy the build scripts
###################################################
COPY scripts/build/* /data/scripts/build/
RUN chmod 777 -R /data/scripts/build/
