FROM joynr-android:latest

###################################################
# install node.js
###################################################
RUN dnf install -y \
	nodejs \
	&& dnf clean all

###################################################
# install libs needed to unpack and run phantomjs
###################################################
RUN dnf install -y \
	bzip2 \
	freetype-devel \
	fontconfig-devel \
	&& dnf clean all

###################################################
# deploy the build scripts
###################################################
COPY scripts/build/* /data/scripts/build/
RUN chmod 777 -R /data/scripts/build/
