#!/bin/bash

# ADDITIONAL_DOCKER_ARGS is provided for external injection
# of required special parameters in order to support extension
# of DNS handling and HOSTS entries via environment settings.
# It is thus required that it does not get assigned any value.

echo "### start build_docker_image.sh for SIT onboard ###"

set -e

DOCKER_BUILDDIR=$(pwd)/target
MAVEN_REPODIR=${HOME}/.m2/repository
MAVEN_SETTINGS=${HOME}/.m2/settings.xml

DOCKER_REPOSITORY=
BASE_DOCKER_IMAGE=joynr-runtime-environment-base:latest
DOCKER_IMAGE_VERSION=latest
DOCKER_RUN_ADD_FLAGS=

JOYNR_REPODIR=$(pwd)/../../../..
SIT_DIR=$(pwd)/../..
CPP_BUILDDIR=$JOYNR_REPODIR/build
JOBS=4
NVM_DIR="/usr/local/nvm"
NODE_V8=8.16.2


function print_usage {
	echo "
Usage: ./build_docker_image.sh [<options>]

Possible options:

--no-cpp-build: skip the clean / build of the joynr CPP framework
--no-cpp-test-build: skip building the sit-cpp-app
--no-java-build: skip building the sit-java-app and the joynr JAVA framework
--no-node-build: skip building the sit-node-app and the joynr JS framework
-r, --docker-repository <docker repository>: set the value of the DOCKER_REPOSITORY variable, determining
	where the cpp base image is pulled from.
-v, --docker-image-version <docker image version>: set the value of the DOCKER_IMAGE_VERSION variable, determining
	where which version of the docker build images (node, cpp) should be used
--docker-run-flags <run flags>: add some additional flags required when calling docker (e.g. \"--sig-proxy -e DEV_UID=$(id -u)\")
--repo-dir <repository directory>: override the default maven repository directory
--maven-settings <maven settings file>: override the location of the default maven settings file
-j, --jobs <number of build jobs>: the number of build jobs used for parallel C++ builds.
-h, --help: print this information

> Note:
> The --no-XYZ-build options can be used to skip building the given artifact
> inside a Docker container (which can be quite slow depending on your system).
> If you do so, then you have to make sure that the build results are available
> by other means - either you have to have built them locally, or you have to
> have the build results still lying around from previously executing this
> script without the relevant '--no-XYZ-build' option.
"
}

while [[ $# -gt 0 ]]
do
	key="$1"
	case $key in
		--no-cpp-build)
		NO_CPP_BUILD=true
		;;
		--no-cpp-test-build)
		NO_CPP_TEST_BUILD=true
		;;
		--no-java-build)
		NO_JAVA_BUILD=true
		;;
		--no-node-build)
		NO_NODE_BUILD=true
		;;
		-r|--docker-repository)
		DOCKER_REPOSITORY="$2"
		shift
		;;
		-v|--docker-image-version)
		DOCKER_IMAGE_VERSION="$2"
		shift
		;;
		--docker-run-flags)
		DOCKER_RUN_ADD_FLAGS="$2"
		shift
		;;
		--repo-dir)
		MAVEN_REPODIR="$2"
		shift
		;;
		--maven-settings)
		MAVEN_SETTINGS="$2"
		shift
		;;
		-j|--jobs)
		shift
		JOBS=$1
		;;
		-h|--help)
		print_usage
		exit 0
		;;
		*)
		echo "Unknown argument: $1"
		print_usage
		exit 1
		;;
	esac
	shift
done

JAVA_CPP_BUILD_DOCKER_IMAGE=joynr-ilt-gcc:${DOCKER_IMAGE_VERSION}
JS_BUILD_DOCKER_IMAGE=joynr-ilt-gcc:${DOCKER_IMAGE_VERSION}

function execute_in_docker {
	if [ -z "$2" ]; then
		DOCKERIMAGE=${JAVA_CPP_BUILD_DOCKER_IMAGE}
	else
		DOCKERIMAGE=$2
	fi
	docker run --rm -t $DOCKER_RUN_ADD_FLAGS --privileged \
		$ADDITIONAL_DOCKER_ARGS \
		-v $JOYNR_REPODIR:/data/src:Z \
		-v $MAVEN_SETTINGS:/home/joynr/.m2/settings.xml:z \
		-v $MAVEN_REPODIR:/home/joynr/.m2/repository:Z \
		-v $CPP_BUILDDIR:/data/build:Z \
		${DOCKER_REPOSITORY}${DOCKERIMAGE} \
		/bin/sh -c "$1"
}

# create cpp build dir:
# The directory has to be created before the first call of 'execute_in_docker' because it is always
# mounted,not only for C++ builds.
mkdir -p $CPP_BUILDDIR

if [ $NO_JAVA_BUILD ]; then
	echo "Skipping Java build ..."
else
	# Build Java
	execute_in_docker '"echo \"Generate Java API\" && . /etc/profile && cd /data/src && mvn clean install -P no-license-and-notice,no-java-formatter,no-checkstyle -DskipTests"'
fi

if [ $NO_CPP_BUILD ]; then
	echo "Skipping C++ build ..."
else
	# assume joynr JAVA and joynr-generator-standalone are already built and C++ code has been generated

	execute_in_docker '"echo \"Building and packaging MoCOCrW\" && . /etc/profile && /data/src/docker/joynr-ilt-gcc/scripts/build/cpp-build-MoCOCrW-package.sh 2>&1"'

	execute_in_docker '"echo \"Building and packaging mosquitto\" && . /etc/profile && /data/src/docker/joynr-ilt-gcc/scripts/build/cpp-build-mosquitto-package.sh 2>&1"'

	execute_in_docker '"echo \"Building and packaging smrf\" && . /etc/profile && /data/src/docker/joynr-ilt-gcc/scripts/build/cpp-build-smrf-rpm-package.sh 2>&1"'

	execute_in_docker '"echo \"Building joynr c++\" && . /etc/profile && /data/src/docker/joynr-ilt-gcc/scripts/build/cpp-clean-build.sh --additionalcmakeargs \"-DUSE_PLATFORM_MUESLI=OFF\" --jobs '"${JOBS}"' --enableclangformatter OFF --buildtests OFF 2>&1"'

	execute_in_docker '"echo \"Packaging joynr c++\" && . /etc/profile && /data/src/docker/joynr-ilt-gcc/scripts/build/cpp-build-rpm-package.sh --rpm-spec tests/system-integration-test/docker/onboard/joynr-without-test.spec 2>&1"'

fi

if [ $NO_CPP_TEST_BUILD ]; then
	echo "Skipping C++ test build ..."
else
	# dummyKeychain is also built here
	execute_in_docker '"echo \"Building C++ System Integration Tests\" && . /etc/profile && export JOYNR_INSTALL_DIR=/data/build/joynr && echo \"dir: \$JOYNR_INSTALL_DIR\" && /data/src/docker/joynr-ilt-gcc/scripts/build/cpp-build-tests.sh system-integration-test --jobs '"${JOBS}"' --clangformatter OFF 2>&1"'
fi

if [ $NO_NODE_BUILD ]; then
	echo "Skipping Node test build ..."
else
	# only build joynr javascript, test-base and sit-node-app
	execute_in_docker '"echo \"Building sit node app\" && . /etc/profile && cd /data/src && mvn clean install -P javascript -am --projects io.joynr.javascript:libjoynr-js,io.joynr.tests:test-base,io.joynr.tests.system-integration-test:sit-node-app && cd tests/system-integration-test/sit-node-app && npm -verbose install"' $JS_BUILD_DOCKER_IMAGE
fi

if [ -d ${DOCKER_BUILDDIR} ]; then
	rm -Rf ${DOCKER_BUILDDIR}
fi
mkdir -p ${DOCKER_BUILDDIR}

# create the directory in any case because it is referenced in Dockerfile below
mkdir ${DOCKER_BUILDDIR}/sit-java-app
cp $SIT_DIR/sit-java-app/target/sit-java-app-*-jar-with-dependencies.jar ${DOCKER_BUILDDIR}/sit-java-app

cp -R -L $SIT_DIR/sit-node-app ${DOCKER_BUILDDIR}
rm ${DOCKER_BUILDDIR}/sit-node-app/node_modules/.bin/ts-node
(
	cd ${DOCKER_BUILDDIR}/sit-node-app/node_modules/.bin
	ln -s ../ts-node/dist/bin.js ts-node
)

cp -R $CPP_BUILDDIR/tests ${DOCKER_BUILDDIR}

cp -R $CPP_BUILDDIR/dummyKeychain ${DOCKER_BUILDDIR}

# Find a file with a name which matches 'joynr-[version].rpm'
find  $CPP_BUILDDIR/joynr/package/RPM/x86_64/ -iregex ".*joynr-[0-9].*rpm" -exec cp {} $DOCKER_BUILDDIR/joynr.rpm \;

# Find a file with a name which matches 'smrf-[version].rpm'
find  $CPP_BUILDDIR/smrf/package/RPM/x86_64/ -iregex ".*smrf-[0-9].*rpm" -exec cp {} $DOCKER_BUILDDIR/smrf.rpm \;

cp $JOYNR_REPODIR/docker/build/MoCOCrW.tar.gz $DOCKER_BUILDDIR/MoCOCrW.tar.gz
cp $JOYNR_REPODIR/docker/build/mosquitto.tar.gz $DOCKER_BUILDDIR/mosquitto.tar.gz

cp -R $JOYNR_REPODIR/docker/joynr-ilt-gcc/scripts/docker/gen-certificates.sh ${DOCKER_BUILDDIR}
cp -R $JOYNR_REPODIR/docker/joynr-ilt-gcc/openssl.conf ${DOCKER_BUILDDIR}

CURRENTDATE=`date`
cp onboard-cc-messaging.settings ${DOCKER_BUILDDIR}
cp onboard-failure-cc-messaging.settings ${DOCKER_BUILDDIR}
cp systemintegrationtest-failure-provider.settings ${DOCKER_BUILDDIR}
cp run-onboard-sit.sh ${DOCKER_BUILDDIR}
cat > $DOCKER_BUILDDIR/Dockerfile <<-EOF
    FROM ${DOCKER_REPOSITORY}${BASE_DOCKER_IMAGE}

    RUN echo "current date: $CURRENTDATE" && . /etc/profile && curl www.google.de > /dev/null

    ###################################################
    # Install boost
    ###################################################
    RUN . /etc/profile \
        && dnf install -y \
        boost \
        libwebsockets \
        openssl

    ###################################################
    # Install MoCOCrW
    ###################################################
    COPY MoCOCrW.tar.gz /tmp/MoCOCrW.tar.gz
    RUN cd / \
        && tar xvf /tmp/MoCOCrW.tar.gz

    ###################################################
    # Install mosquitto
    ###################################################
    COPY mosquitto.tar.gz /tmp/mosquitto.tar.gz
    RUN cd / \
        && tar xvf /tmp/mosquitto.tar.gz

    ###################################################
    # Install joynr and smrf
    ###################################################
    COPY joynr.rpm /tmp/joynr.rpm
    COPY smrf.rpm /tmp/smrf.rpm
    RUN rpm -i --nodeps \
        /tmp/smrf.rpm \
        /tmp/joynr.rpm \
        && rm /tmp/joynr.rpm
    RUN echo "/usr/local/lib64" > /etc/ld.so.conf.d/joynr.conf && ldconfig

    ###################################################
    # Copy C++ binaries
    ###################################################
    COPY tests /data/sit-cpp-app

    ###################################################
    # Copy libdummyKeyChain
    ###################################################
    COPY dummyKeychain /data/build/dummyKeychain

    ###################################################
    # Copy sit-node-app
    ###################################################
    COPY sit-node-app /data/sit-node-app

    ###################################################
    # Copy sit-java-app
    ###################################################
    COPY sit-java-app /data/sit-java-app

    ###################################################
    # Generate certificates
    ###################################################
    COPY gen-certificates.sh /data/scripts/gen-certificates.sh
    COPY openssl.conf /tmp/openssl.cnf
    RUN mkdir -p /data/ssl-data \
    && /data/scripts/gen-certificates.sh --configfile /tmp/openssl.cnf --destdir /data/ssl-data

    ###################################################
    # install node.js
    ###################################################
    # nvm environment variables
    ENV NVM_DIR $NVM_DIR

    ENV NODE_V8 $NODE_V8

    # install nvm
    RUN . /etc/profile \
        && mkdir -p $NVM_DIR \
        && curl --silent -o- https://raw.githubusercontent.com/creationix/nvm/v0.39.1/install.sh | bash

    # install node and npm
    # having the nvm directory writable makes it possible to use nvm to change node versions manually
    RUN . /etc/profile \
        && source $NVM_DIR/nvm.sh \
        && nvm install $NODE_V8 \
        && nvm alias default $NODE_V8 \
        && nvm use default \
        && chmod -R a+rwx $NVM_DIR

    # add node and npm to path
    # (node will be available then without sourcing $NVM_DIR/nvm.sh)
    ENV PATH $NVM_DIR/versions/node/v$NODE_V8/bin:$PATH

    ###################################################
    # Copy run script
    ###################################################
    COPY onboard-cc-messaging.settings /data/onboard-cc-messaging.settings
    COPY onboard-failure-cc-messaging.settings /data/onboard-failure-cc-messaging.settings
    COPY systemintegrationtest-failure-provider.settings /data/sit-cpp-app/resources/systemintegrationtest-failure-provider.settings
    COPY run-onboard-sit.sh /data/run-onboard-sit.sh

    ENTRYPOINT ["sh", "-c", "\"/data/run-onboard-sit.sh\""]
EOF
chmod 666 $DOCKER_BUILDDIR/Dockerfile

echo "environment:" `env`
echo "docker build -t sit-onboard-apps:latest $DOCKER_BUILDDIR"
docker build -t sit-onboard-apps:latest $DOCKER_BUILDDIR

echo "### end build_docker_image.sh for joynr-gcd ###"
rm -rf $DOCKER_BUILDDIR

echo "### end build_docker_image.sh for onboard ###"
