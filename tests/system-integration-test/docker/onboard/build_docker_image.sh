#!/bin/bash

# ADDITIONAL_DOCKER_ARGS is provided for external injection
# of required special parameters in order to support extension
# of DNS handling and HOSTS entries via environment settings.
# It is thus required that it does not get assigned any value.

echo "### start build_docker_image.sh for onboard ###"

BUILDDIR=target
REPODIR=${HOME}/.m2/repository
DOCKER_REPOSITORY=
MAVENSETTINGS=${HOME}/.m2/settings.xml
BASE_DOCKER_IMAGE=joynr-runtime-environment-base:latest
DOCKER_IMAGE_VERSION=latest
DOCKER_RUN_ADD_FLAGS=
JOBS=4

# The --no-XYZ-build options can be used to skip building the given artifact
# inside a Docker container (which can be quite slow depending on your system).
# If you do so, then you have to make sure that the build results are available
# by other means - either you have to have built them locally, or you have to
# have the build results still lying around from previously executing this
# script without the relevant '--no-XYZ-build' option.

function print_usage {
	echo "
Usage: ./build_docker_image.sh [<options>]

Possible options:

--no-cpp-build: skip the clean / build of the joynr CPP framework
--no-cpp-test-build: skip building the sit-cpp-app
--no-java-test-build: skip building the sit-java-app
--no-node-test-build: skip building the sit-node-app
-r, --docker-repository <docker repository>: set the value of the DOCKER_REPOSITORY variable, determining
    where the cpp base image is pulled from.
-v, --docker-image-version <docker image version>: set the value of the DOCKER_IMAGE_VERSION variable, determining
    where which version of the docker build images (node, cpp) should be used
--docker-run-flags <run flags>: add some additional flags required when calling docker (e.g. \"--sig-proxy -e DEV_UID=$(id -u)\")
--repo-dir <repository directory>: override the default maven repository directory
--maven-settings <maven settings file>: override the location of the default maven settings file
-j, --jobs <number of build jobs>: the number of build jobs used for parallel C++ builds.
-h, --help: print this information
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
		--no-java-test-build)
		NO_JAVA_TEST_BUILD=true
		;;
		--no-node-test-build)
		NO_NODE_TEST_BUILD=true
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
		REPODIR="$2"
		shift
		;;
		--maven-settings)
		MAVENSETTINGS="$2"
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
		exit -1
		;;
	esac
	shift
done

CPP_BUILD_DOCKER_IMAGE=joynr-cpp-gcc:${DOCKER_IMAGE_VERSION}
JS_BUILD_DOCKER_IMAGE=joynr-ilt-gcc:${DOCKER_IMAGE_VERSION}

function execute_in_docker {
	if [ -z "$2" ]; then
		DOCKERIMAGE=${CPP_BUILD_DOCKER_IMAGE}
	else
		DOCKERIMAGE=$2
	fi
	docker run --rm -t $DOCKER_RUN_ADD_FLAGS --privileged \
		$ADDITIONAL_DOCKER_ARGS \
		-e http_proxy=$http_proxy \
		-e https_proxy=$https_proxy \
		-e no_proxy=$no_proxy \
		-v $(pwd)/../../../..:/data/src:Z \
		-v $MAVENSETTINGS:/home/joynr/.m2/settings.xml:z \
		-v $REPODIR:/home/joynr/.m2/repository:Z \
		-v $(pwd)/../../../../build:/data/build:Z \
		${DOCKER_REPOSITORY}${DOCKERIMAGE} \
		/bin/sh -c "$1"
}

#create build dir:
mkdir -p $(pwd)/../../../../build

if [ $NO_JAVA_TEST_BUILD ]; then
	echo "Skipping Java build ..."
else
	# Build Java
	execute_in_docker '"echo \"Generate Java API\" && cd /data/src && mvn clean install -P no-license-and-notice,no-java-formatter,no-checkstyle -DskipTests"'
	SKIP_MAVEN_BUILD_CPP_PREREQUISITES=true
fi

if [ $NO_CPP_BUILD ]; then
	echo "Skipping C++ build ..."
else
	# if Java is included, the following section can be skipped since already included above
	if [ -z $SKIP_MAVEN_BUILD_CPP_PREREQUISITES ]
	then
		execute_in_docker '"echo \"Generate joynr C++ API\" && cd /data/src && mvn clean install -P no-license-and-notice,no-java-formatter,no-checkstyle -DskipTests -am\
		--projects io.joynr:basemodel,io.joynr.tools.generator:dependency-libs,io.joynr.tools.generator:generator-framework,io.joynr.tools.generator:joynr-generator-maven-plugin,io.joynr.tools.generator:cpp-generator,io.joynr.cpp:libjoynr,io.joynr.tools.generator:joynr-generator-standalone"'
	fi

	execute_in_docker '"echo \"Building and packaging smrf\" && /data/src/docker/joynr-cpp-base/scripts/build/cpp-build-smrf-rpm-package.sh 2>&1"'

	execute_in_docker '"echo \"Building joynr c++\" && /data/src/docker/joynr-cpp-base/scripts/build/cpp-clean-build.sh --additionalcmakeargs \"-DUSE_PLATFORM_MUESLI=OFF\" --jobs '"${JOBS}"' --enableclangformatter OFF --buildtests OFF 2>&1"'

	execute_in_docker '"echo \"Packaging joynr c++\" && /data/src/docker/joynr-cpp-base/scripts/build/cpp-build-rpm-package.sh --rpm-spec tests/system-integration-test/docker/onboard/joynr-without-test.spec 2>&1"'

	execute_in_docker '"echo \"Building MoCOCrW tarball\" && /data/src/docker/joynr-cpp-base/scripts/build/cpp-create-MoCOCrW-tarball.sh 2>&1"'

fi

if [ $NO_CPP_TEST_BUILD ]; then
	echo "Skipping C++ test build ..."
else
	execute_in_docker '"echo \"Building C++ System Integration Tests\" && export JOYNR_INSTALL_DIR=/data/build/joynr && echo \"dir: \$JOYNR_INSTALL_DIR\" && /data/src/docker/joynr-cpp-base/scripts/build/cpp-build-tests.sh system-integration-test --jobs '"${JOBS}"' --clangformatter OFF 2>&1"'
fi

if [ $NO_NODE_TEST_BUILD ]; then
	echo "Skipping Node test build ..."
else
	execute_in_docker '"echo \"Building sit node app\" && cd /data/src && mvn clean install -P javascript -am --projects io.joynr.javascript:libjoynr-js,io.joynr.tests:test-base,io.joynr.tests.system-integration-test:sit-node-app && cd tests/system-integration-test/sit-node-app && npm install"' $JS_BUILD_DOCKER_IMAGE
fi

if [ -d ${BUILDDIR} ]; then
	rm -Rf ${BUILDDIR}
fi
mkdir -p ${BUILDDIR}

cp -R ../../sit-node-app ${BUILDDIR}
cp -R ../../../../build/tests ${BUILDDIR}
# create the directory in any case because it is referenced in Dockerfile below
mkdir ${BUILDDIR}/sit-java-app
cp ../../sit-java-app/target/sit-java-app-*-jar-with-dependencies.jar ${BUILDDIR}/sit-java-app

# Find a file with a name which matches 'joynr-[version].rpm'
find  ../../../../build/joynr/package/RPM/x86_64/ -iregex ".*joynr-[0-9].*rpm" -exec cp {} $BUILDDIR/joynr.rpm \;

# Find a file with a name which matches 'smrf-[version].rpm'
find  ../../../../build/smrf/package/RPM/x86_64/ -iregex ".*smrf-[0-9].*rpm" -exec cp {} $BUILDDIR/smrf.rpm \;

cp ../../../../build/MoCOCrW.tar.gz $BUILDDIR/MoCOCrW.tar.gz

CURRENTDATE=`date`
cp onboard-cc-messaging.settings ${BUILDDIR}
cp run-onboard-sit.sh ${BUILDDIR}
cat > $BUILDDIR/Dockerfile <<-EOF
    FROM ${DOCKER_REPOSITORY}${BASE_DOCKER_IMAGE}

    RUN echo "current date: $CURRENTDATE" && curl www.google.de > /dev/null

    ###################################################
    # Install boost
    ###################################################
    RUN dnf install -y \
        boost \
        mosquitto

    ###################################################
    # Install MoCOCrW
    ###################################################
    COPY MoCOCrW.tar.gz /tmp/MoCOCrW.tar.gz
    RUN cd / && \
        tar xvf /tmp/MoCOCrW.tar.gz

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
    # Copy sit-node-app
    ###################################################
    COPY sit-node-app /data/sit-node-app

    ###################################################
    # Copy sit-java-app
    ###################################################
    COPY sit-java-app /data/sit-java-app

    ###################################################
    # Copy run script
    ###################################################
    COPY onboard-cc-messaging.settings /data/onboard-cc-messaging.settings
    COPY run-onboard-sit.sh /data/run-onboard-sit.sh

    ENTRYPOINT ["sh", "-c", "\"/data/run-onboard-sit.sh\""]
EOF
chmod 666 $BUILDDIR/Dockerfile

echo "environment:" `env`
echo "docker build -t sit-apps:latest --build-arg http_proxy=${http_proxy} --build-arg https_proxy=${https_proxy} --build-arg no_proxy=${no_proxy} $BUILDDIR"
docker build -t sit-apps:latest --build-arg http_proxy=${http_proxy} --build-arg https_proxy=${https_proxy} --build-arg no_proxy=${no_proxy} $BUILDDIR

docker images --filter "dangling=true" -q | xargs docker rmi -f 2>/dev/null
rm -Rf $BUILDDIR

echo "### end build_docker_image.sh for onboard ###"
