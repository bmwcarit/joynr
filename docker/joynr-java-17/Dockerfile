FROM joynr-base:latest

###################################################
# uninstall java 11
###################################################
RUN dnf remove -y java-11-openjdk-devel \
    java-11-openjdk \
    java-11-openjdk-headless \
    && dnf -y autoremove

###################################################
# configure Java 17 as default
###################################################
RUN update-alternatives --set java /usr/lib/jvm/java-17-openjdk*.x86_64/bin/java \
    && update-alternatives --set javac /usr/lib/jvm/java-17-openjdk*.x86_64/bin/javac \
    && update-alternatives --set jre_openjdk /usr/lib/jvm/java-17-openjdk*.x86_64 \
    && update-alternatives --set java_sdk_openjdk /usr/lib/jvm/java-17-openjdk*.x86_64

###################################################
# Patch Java security configuration so that SHA1
# signed classes are still accepted. This is
# required for old plugins / dependencies required
# by joynr generator which need to stay compatible
# with Java 8 and thus cannot be updated.
###################################################
RUN mkdir -p /tmp/patches/java
COPY patches/java /tmp/patches/java
RUN cd /usr/lib/jvm/java-17/conf/security \
    && patch < /tmp/patches/java/java-17.security.patch \
    && rm -fr /tmp/patches/java
