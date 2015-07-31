FROM joynr-java:latest

USER root
###################################################
# install android sdk
###################################################
RUN yum  install -y \
    glibc.i686 \
    glibc-devel.i686 \
    libstdc++.i686 \
    zlib-devel.i686 \
    ncurses-devel.i686 \
    && yum clean all

ENV ANDROID_SDK_FILENAME android-sdk_r24.1.2-linux.tgz
ENV ANDROID_SDK_URL http://dl.google.com/android/${ANDROID_SDK_FILENAME}
ENV ANDROID_API_LEVELS android-19,android-21
ENV ANDROID_BUILD_TOOLS_VERSION 22.0.1
ENV ANDROID_HOME /opt/android-sdk-linux
ENV PATH ${PATH}:${ANDROID_HOME}/tools:${ANDROID_HOME}/platform-tools
RUN cd /opt \
    && wget -q ${ANDROID_SDK_URL} \
    && tar -xzf ${ANDROID_SDK_FILENAME} \
    && rm ${ANDROID_SDK_FILENAME}

RUN chown -R root:root ${ANDROID_HOME} \
    && chmod -R 755 ${ANDROID_HOME}

# set this date to cause android to be updated
ENV REFRESHED_ANDROID_AT 2015-05-13
RUN echo y | android update sdk --no-ui -a --filter ${ANDROID_API_LEVELS}
RUN echo y | android update sdk --no-ui -a --filter build-tools-${ANDROID_BUILD_TOOLS_VERSION}
RUN echo y | android update sdk --no-ui -a --filter platform-tools

COPY scripts/build/* /data/scripts/build/
RUN chmod 777 -R /data/scripts/build/

RUN date -R > /data/timestamp
