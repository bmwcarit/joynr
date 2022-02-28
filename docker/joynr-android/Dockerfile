FROM joynr-base:latest

USER root
###################################################
# install android sdk
###################################################
RUN dnf install -y \
    glibc.i686 \
    glibc-devel.i686 \
    libstdc++.i686 \
    zlib-devel.i686 \
    ncurses-devel.i686 \
    mesa-libGLU.i686 \
    && dnf clean all

COPY scripts/build/setup-android.sh /data/scripts/build/setup-android.sh
RUN chmod 777 -R /data/scripts/build/

ENV ANDROID_SDK_FILENAME commandlinetools-linux-7583922_latest.zip
ENV ANDROID_SDK_URL https://dl.google.com/android/repository/${ANDROID_SDK_FILENAME}
ENV ANDROID_API_LEVEL android-28
ENV ANDROID_BUILD_TOOLS_VERSION 29.0.3
ENV ANDROID_HOME /opt/android-sdk-linux
ENV PATH ${PATH}:${ANDROID_HOME}/tools:${ANDROID_HOME}/platform-tools
RUN cd /opt \
    && mkdir -p ${ANDROID_HOME} \
    && cd ${ANDROID_HOME} \
    && wget -q ${ANDROID_SDK_URL} \
    && unzip ${ANDROID_SDK_FILENAME} \
    && rm ${ANDROID_SDK_FILENAME} \
    && mv cmdline-tools tmp-cmdline-tools \
    && mkdir -p ${ANDROID_HOME}/cmdline-tools/latest \
    && mv tmp-cmdline-tools/* ${ANDROID_HOME}/cmdline-tools/latest \
    && rm -fr tmp-cmdline-tools

RUN chown -R root:root ${ANDROID_HOME} \
    && chmod -R 755 ${ANDROID_HOME}
# set this date to cause android to be updated
ENV REFRESHED_ANDROID_AT 2020-07-08
RUN /data/scripts/build/setup-android.sh

RUN date -R > /data/timestamp
