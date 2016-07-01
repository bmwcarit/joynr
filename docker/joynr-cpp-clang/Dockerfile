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
    && ./b2 variant=release --without-wave --without-python --without-mpi --without-iostreams --without-graph_parallel -j"$(nproc)" install \
    && rm -rf /opt/boost

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

