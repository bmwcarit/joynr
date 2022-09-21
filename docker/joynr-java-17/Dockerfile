FROM joynr-base:latest

###################################################
# configure Java 17 as default
###################################################
RUN update-alternatives --set java /usr/lib/jvm/java-17-openjdk*.x86_64/bin/java \
    && update-alternatives --set javac /usr/lib/jvm/java-17-openjdk*.x86_64/bin/javac \
    && update-alternatives --set jre_openjdk /usr/lib/jvm/java-17-openjdk*.x86_64 \
    && update-alternatives --set java_sdk_openjdk /usr/lib/jvm/java-17-openjdk*.x86_64
