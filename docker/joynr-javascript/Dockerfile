FROM joynr-android:latest

###################################################
# install node.js
###################################################
RUN dnf install -y \
	nodejs \
	npm \
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
# install phantomjs to avoid additional download
###################################################
RUN npm install -g phantomjs-prebuilt@2.1.12

###################################################
# deploy the build scripts
###################################################
COPY scripts/build/* /data/scripts/build/
RUN chmod 777 -R /data/scripts/build/
