FROM joynr-base:latest

ENV PKG_CONFIG_PATH /usr/local/lib/pkgconfig:$PKG_CONFIG_PATH

###################################################
# install qt 5.3.1
###################################################
WORKDIR /opt
RUN wget http://download.qt.io/archive/qt/5.3/5.3.2/single/qt-everywhere-opensource-src-5.3.2.tar.gz
RUN mkdir -p /opt/qt5 \
	&& tar -xf qt-everywhere-opensource-src-5.3.2.tar.gz -C /opt/qt5 --strip-components=1 \
	&& rm qt-everywhere-opensource-src-5.3.2.tar.gz
RUN cd /opt/qt5 \
	&&./configure --prefix=/usr/local \
		-developer-build \
		-opensource \
		-nomake examples \
		-nomake tests \
		-no-gtkstyle \
		-confirm-license \
		-openssl \
		-skip qtactiveqt \
		-skip qtandroidextras \
		-skip qtconnectivity \
		-skip qtdeclarative \
		-skip qtdoc \
		-skip qtenginio \
		-skip qtgraphicaleffects \
		-skip qtimageformats \
		-skip qtlocation \
		-skip qtmacextras \
		-skip qtmultimedia \
		-skip qtquick1 \
		-skip qtquickcontrols \
		-skip qtsensors \
		-skip qtserialport \
		-skip qttranslations \
		-skip qtwebkit \
		-skip qtwebkit-examples \
		-skip qtwinextras \
		-skip qtx11extras \
		-v \
	&& make -j"$(nproc)" all \
	&& make install \
	&& rm -rf /opt/qt5


###################################################
# install boost
###################################################
WORKDIR /opt

RUN curl -L  http://downloads.sourceforge.net/project/boost/boost/1.56.0/boost_1_56_0.tar.gz > boost.tar.gz \
	&& mkdir -p /opt/boost \
	&& tar -zxf boost.tar.gz -C /opt/boost --strip-components=1 \
	&& rm boost.tar.gz

RUN cd /opt/boost \
	&& ./bootstrap.sh --prefix=/usr/local \
	&& ./b2 --without-wave --without-python --without-mpi --without-iostreams --without-graph_parallel -j"$(nproc)" install \
	&& rm -rf /opt/boost


###################################################
# install common api
###################################################
RUN cd /opt \
	###### clone common-api-dbus-runtime and
	###### make sure common-api-dbus-runtimechecked out at
	###### correct version, so that patch is applied correctly
	&& git clone http://git.projects.genivi.org/ipc/common-api-dbus-runtime.git \
	&& cd /opt/common-api-dbus-runtime \
	&& git checkout -b v2.1.4 2.1.4 \
	###### installing CommonAPI base
	&& cd /opt \
	&& git clone http://git.projects.genivi.org/ipc/common-api-runtime.git \
	&& cd /opt/common-api-runtime \
	&& git checkout -b v2.1.4 2.1.4 \
	&& autoreconf -i \
	&& ./configure \
	&& make -j"$(nproc)" \
	&& make install \
	&& cd /opt \
	&& rm -rf /opt/common-api-runtime \
	###### installing dbus for CommonAPI
	&& cd /opt \
	&& wget http://dbus.freedesktop.org/releases/dbus/dbus-1.4.16.tar.gz \
	&& tar -xzf dbus-1.4.16.tar.gz \
	&& rm dbus-1.4.16.tar.gz \
	&& cd /opt/dbus-1.4.16 \
	&& patch -p1 < ../common-api-dbus-runtime/dbus-DBusMessage-add-support-for-custom-marshaling.patch \
	&& ./configure --prefix=/usr/local \
	&& make -j"$(nproc)" -C dbus \
	&& make -C dbus install \
	&& make install-pkgconfigDATA \
	&& cd /opt \
	&& rm -rf /opt/dbus-1.4.16 \
	###### installing CommonAPI dbus
	&& cd /opt/common-api-dbus-runtime \
	&& autoreconf -i \
	&& export PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH" && ./configure \
	&& make -j"$(nproc)" \
	&& make install \
	&& cd /opt \
	&& rm -rf /opt/common-api-dbus-runtime

ENV PKG_CONFIG_PATH="/opt/common-api/common-api-2.1.4/lib/pkgconfig:$PKG_CONFIG_PATH"

###################################################
# install clang 3.5 for formatting
###################################################
RUN yum update -y \
	&& yum install -y \
	clang-devel \
	&& yum clean all

###################################################
# install gcovr for code coverage reports
###################################################
RUN yum update -y \
	&& yum install -y \
	python-pip \
	&& yum clean all

###################################################
# install lcov
###################################################
RUN yum update -y \
	&& yum install -y \
	lcov \
	&& yum clean all

RUN pip install gcovr

###################################################
# install libwebsockets
###################################################

RUN cd /opt \
    && git clone git://git.libwebsockets.org/libwebsockets \
    && cd libwebsockets \
    && git checkout v1.5-chrome47-firefox41 \
    && mkdir build \
    && cd build \
    && cmake .. \
    && make install -j"$(nproc)" \
    && cd /opt/ \
    && rm -rf libwebsockets

###################################################
# install jsmn
###################################################

RUN cd /opt \
    && git clone https://github.com/zserge/jsmn.git \
    && cd jsmn \
    && git checkout 572ace5 \
    && CFLAGS=-fPIC make \
    && cp libjsmn.a /usr/local/lib \
    && cp jsmn.h /usr/local/include \
    && cd /opt/ \
    && rm -rf jsmn

###################################################
# install spdlog
###################################################

RUN cd /opt \
    && git clone https://github.com/gabime/spdlog.git \
    && cd spdlog \
    && git checkout e91e1b8 \
    && mkdir build \
    && cd build \
    && cmake .. \
    && make install -j"$(nproc)" \
    && cd /opt/ \
    && rm -rf spdlog

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

###################################################
# Copy build scripts
###################################################
COPY scripts /data/scripts

###################################################
# setup build environment
###################################################
RUN echo "source /data/scripts/bashrc.sh" >> /home/joynr/.bashrc
RUN mkdir /home/joynr/build
RUN date -R > /data/timestamp
