FROM joynr-cpp-base:latest

###################################################
# install gcovr for code coverage reports
###################################################
RUN dnf update -y \
    && dnf install -y \
    python-pip \
    && dnf clean all \
    && pip install gcovr

###################################################
# install lcov
###################################################
RUN dnf update -y \
    && dnf install -y \
    lcov \
    && dnf clean all

###################################################
# install boost
###################################################
RUN dnf update -y \
    && dnf install -y \
    boost \
    boost-devel \
    && dnf clean all

###################################################
# install googletest & googlemock
###################################################

RUN cd /opt \
    && git clone https://github.com/google/googletest.git \
    && cd googletest \
    && git checkout ddb8012e \
    && mkdir build \
    && cd build \
    && cmake .. \
    && make install -j"$(nproc)" \
    && cd /opt/ \
    && rm -rf googletest
