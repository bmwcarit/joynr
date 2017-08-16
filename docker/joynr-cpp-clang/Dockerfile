FROM joynr-cpp-base:latest

###################################################
# install boost
###################################################

WORKDIR /opt

# Use the same boost version which is provided by the fedora-24 repository
RUN curl -L  http://sourceforge.net/projects/boost/files/boost/1.60.0/boost_1_60_0.tar.gz > boost.tar.gz \
    && mkdir -p /opt/boost \
    && tar -zxf boost.tar.gz -C /opt/boost --strip-components=1 \
    && rm boost.tar.gz \
    && cd /opt/boost \
    && ./bootstrap.sh --with-toolset=clang --prefix=/usr/local \
    && ./b2 variant=release -s NO_BZIP2=1 --without-wave --without-python --without-mpi --without-graph_parallel -j"$(nproc)" install \
    && rm -rf /opt/boost

###################################################
# install rapidjson
###################################################

RUN cd /opt \
    && git clone https://github.com/miloyip/rapidjson.git rapidjson \
    && cd rapidjson \
    && git checkout v1.1.0 \
    && mkdir build \
    && cd build \
    && cmake -DRAPIDJSON_BUILD_DOC=OFF \
    -DRAPIDJSON_BUILD_EXAMPLES=OFF \
    -DRAPIDJSON_BUILD_TESTS=OFF \
    -DRAPIDJSON_BUILD_THIRDPARTY_GTEST=OFF .. \
    && make install -j"$(nproc)" \
    && cd /opt \
    && rm -rf rapidjson

###################################################
# install muesli
###################################################

RUN cd /opt \
    && git clone https://github.com/bmwcarit/muesli.git \
    && cd muesli \
    && git checkout 0.3.1 \
    && mkdir build \
    && cd build \
    && cmake -DBUILD_MUESLI_TESTS=Off -DUSE_PLATFORM_RAPIDJSON=On .. \
    && make install -j"$(nproc)" \
    && cd /opt \
    && rm -rf muesli

###################################################
# install googletest & googlemock
###################################################

RUN cd /opt \
    && git clone https://github.com/google/googletest.git \
    && cd googletest \
    && git checkout ddb8012e \
    && mkdir build \
    && cd build \
    && cmake -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++ .. \
    && make install -j"$(nproc)" \
    && cd /opt/ \
    && rm -rf googletest

###################################################
# install smrf
###################################################

RUN export SMRF_VERSION=0.2.1 \
    && cd /opt \
    && git clone https://github.com/bmwcarit/smrf.git \
    && cd smrf \
    && git checkout $SMRF_VERSION \
    && mkdir build \
    && cd build \
    && cmake -DBUILD_TESTS=Off .. \
    && make install -j"$(nproc)" \
    && cd /opt \
    && rm -rf smrf
