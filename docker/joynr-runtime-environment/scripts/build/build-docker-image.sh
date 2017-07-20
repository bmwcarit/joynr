#!/bin/bash

function usage
{
    echo "usage: build-docker-image --version versionnumber --joynrrpm rpmname"
    echo "       --testrpm testrpmname"
    echo "       --smrfrpm smrfrpmname"
    echo "       [--repository mydockerrepo.org]"
    echo "Must be called from the joynr source directory"
}

repository=
version=
joynrrpm=
smrfrpm=
testrpm=
dockerbuildargs=
http_proxy=""
https_proxy=""
no_proxy=""
push_image=OFF

while [ "$1" != "" ]; do
echo "PARAM is: $1"
    case $1 in
        -r | --repository )        shift
                                   echo "REPO"
                                   repository=${1%/}/
                                   ;;
        -v | --version )           shift
                                   version=":$1"
                                   ;;
        -j | --joynrrpm )          shift
                                   joynrrpm="$1"
                                   ;;
        -s | --smrfrpm )           shift
                                   smrfrpm="$1"
                                   ;;
        -t | --testrpm )           shift
                                   testrpm="$1"
                                   ;;
        --http_proxy )             shift
                                   http_proxy="$1"
                                   ;;
        --https_proxy )            shift
                                   https_proxy="$1"
                                   ;;
        --no_proxy )               shift
                                   no_proxy="$1"
                                   ;;
        --push_image )             shift
                                   push_image="$1"
                                   ;;
        -h | --help )              usage
                                   exit
                                   ;;
        * )                        usage
                                   exit 1
    esac
    shift
done

FILE_DIR=docker/joynr-runtime-environment/files

rm -rf ${FILE_DIR}
mkdir -p ${FILE_DIR}/performance-test/

cp -R javascript/libjoynr-js ${FILE_DIR}/libjoynr-js/
cp -R tests/test-base ${FILE_DIR}/test-base/
cp -R java/backend-services/discovery-directory-servlet/target/discovery-directory-servlet.war ${FILE_DIR}/discovery-directory-servlet.war
cp java/backend-services/domain-access-controller-servlet/target/domain-access-controller-servlet.war ${FILE_DIR}/domain-access-controller-servlet.war
cp java/messaging/bounceproxy/single-bounceproxy/target/single-bounceproxy.war ${FILE_DIR}/single-bounceproxy.war
cp -R tests/performance-test/src ${FILE_DIR}/performance-test/src
cp tests/performance-test/package.json ${FILE_DIR}/performance-test/package.json
cp tests/performance-test/run-performance-tests.sh ${FILE_DIR}/performance-test/run-performance-tests.sh
cp tests/resources/mosquitto.conf ${FILE_DIR}/mosquitto.conf

# We need to remove the version string (example: performance-test-<JOYNR_VERSION>.jar)
# from the consumer and provider jar file
find tests/performance-test/target/ -iregex ".*performance-test-consumer.*jar" -exec cp {} ${FILE_DIR}/performance-test/performance-test-consumer.jar \;
find tests/performance-test/target/ -iregex ".*performance-test-provider.*jar" -exec cp {} ${FILE_DIR}/performance-test/performance-test-provider.jar \;

(
cd docker/joynr-runtime-environment/
docker build \
    -t ${repository}joynr-runtime-environment${version} \
    --build-arg http_proxy=${http_proxy} \
    --build-arg https_proxy=${https_proxy} \
    --build-arg no_proxy=${no_proxy} \
    --build-arg SMRF_RPM_NAME=${smrfrpm} \
    --build-arg JOYNR_RPM_NAME=${joynrrpm} \
    --build-arg JOYNR_TEST_RPM_NAME=${testrpm} \
    .

docker tag ${repository}joynr-runtime-environment${version} ${repository}joynr-runtime-environment:latest
docker tag ${repository}joynr-runtime-environment${version} joynr-runtime-environment:latest

if [ "$push_image" == "ON" ]
then
docker push ${repository}joynr-runtime-environment${version}
fi

)

rm -rf ${FILE_DIR}/
