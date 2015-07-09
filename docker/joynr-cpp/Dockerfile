FROM joynr-base:latest

ENV PKG_CONFIG_PATH /usr/local/lib/pkgconfig:$PKG_CONFIG_PATH

###################################################
# install qt 5.3.1
###################################################
WORKDIR /opt
RUN wget https://dl.bmw-carit.de/joynr/qt5.tar.gz \
	&& tar -xf qt5.tar.gz \
	&& rm qt5.tar.gz \
	&& cd /opt/qt5 \
	&& ./configure --prefix=/usr/local \
		-developer-build \
		-opensource \
		-nomake examples \
		-nomake tests \
		-no-gtkstyle \
		-confirm-license \
		-openssl \
		-skip qt3d \
		-skip qtactiveqt \
		-skip qtandroidextras \
		-skip qtconnectivity \
		-skip qtdeclarative \
		-skip qtdoc \
		-skip qtdocgallery \
		-skip qtenginio \
		-skip qtfeedback \
		-skip qtgraphicaleffects \
		-skip qtimageformats \
		-skip qtjsondb \
		-skip qtlocation \
		-skip qtmacextras \
		-skip qtmultimedia \
		-skip qtpim \
		-skip qtqa \
		-skip qtquick1 \
		-skip qtquickcontrols \
		-skip qtrepotools \
		-skip qtsensors \
		-skip qtserialport \
		-skip qtsystems \
		-skip qttranslations \
		-skip qtwayland \
		-skip qtwebkit \
		-skip qtwebkit-examples \
		-skip qtwinextras \
		-skip qtx11extras \
		-v \
	&& make -j8 all \
	&& make install \
	&& rm -rf /opt/qt5

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
	&& make -j4 \
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
	&& make -j4 -C dbus \
	&& make -C dbus install \
	&& make install-pkgconfigDATA \
	&& cd /opt \
	&& rm -rf /opt/dbus-1.4.16 \
	###### installing CommonAPI dbus
	&& cd /opt/common-api-dbus-runtime \
	&& autoreconf -i \
	&& export PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH" && ./configure \
	&& make -j4 \
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

RUN pip install gcovr
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