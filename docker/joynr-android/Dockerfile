FROM joynr-java:latest

USER root
###################################################
# install android sdk
###################################################
RUN yum  install -y \
#	ncurses-libs.i686 \
#	zlib.i686 \
	glibc.i686 \
	glibc-devel.i686 \
	libstdc++.i686 \
	zlib-devel.i686 \
	ncurses-devel.i686 \
    && yum clean all

ENV ANDROID_SDK_FILENAME android-sdk_r24.1.2-linux.tgz
ENV ANDROID_SDK_URL http://dl.google.com/android/${ANDROID_SDK_FILENAME}
ENV ANDROID_API_LEVELS android-19
ENV ANDROID_BUILD_TOOLS_VERSION 22.0.1
ENV ANDROID_HOME /opt/android-sdk-linux
ENV PATH ${PATH}:${ANDROID_HOME}/tools:${ANDROID_HOME}/platform-tools
RUN cd /opt \
    && wget -q ${ANDROID_SDK_URL} \
    && tar -xzf ${ANDROID_SDK_FILENAME} \
    && rm ${ANDROID_SDK_FILENAME}

# set this date to cause android to be updated
ENV REFRESHED_ANDROID_AT 2015-04-21
RUN echo y | android update sdk --no-ui -a --filter tools,platform-tools,${ANDROID_API_LEVELS},build-tools-${ANDROID_BUILD_TOOLS_VERSION}
RUN chown -R root:root ${ANDROID_HOME} \
    && chmod -R 755 ${ANDROID_HOME}

RUN date -R > /data/timestamp
