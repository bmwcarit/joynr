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
