FROM joynr-base:latest

###################################################
# uninstall java 11 and install java 8
###################################################
RUN dnf remove -y java-11-openjdk \
    java-11-openjdk-headless \
    && dnf -y autoremove

RUN dnf install -y \
    java-1.8.0-openjdk \
    java-1.8.0-openjdk-devel \
    && dnf clean all

###################################################
# configure Java 8 as default
#
# keep in mind that java 17 is default and still
# installed. An attempt to remove it uninstalls
# several other packages like maven etc.
###################################################
RUN alternatives --set java /usr/lib/jvm/java-1.8.0-openjdk-1.8.0.322.b06-6.fc36.x86_64/jre/bin/java \
    && alternatives --set javac /usr/lib/jvm/java-1.8.0-openjdk-1.8.0.322.b06-6.fc36.x86_64/bin/javac \
    && alternatives --set jre_openjdk /usr/lib/jvm/java-1.8.0-openjdk-1.8.0.322.b06-6.fc36.x86_64/jre \
    && alternatives --set java_sdk_openjdk /usr/lib/jvm/java-1.8.0-openjdk-1.8.0.322.b06-6.fc36.x86_64
