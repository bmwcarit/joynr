FROM fedora:24

LABEL com.jfrog.artifactory.retention.maxCount="25"

ARG JOYNR_VERSION

#install joynr from rpm
RUN dnf upgrade -y \
    && dnf install -y https://github.com/bmwcarit/joynr-repos/raw/master/joynr-$JOYNR_VERSION-1.x86_64.rpm \
    && dnf clean all

EXPOSE 4242
ENTRYPOINT ["cluster-controller"]
