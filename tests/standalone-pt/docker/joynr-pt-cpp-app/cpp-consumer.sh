#!/bin/bash
set -x
. /etc/profile

/data/src/docker/joynr-cpp-base/scripts/build/cpp-build-tests.sh standalone-pt --jobs 16

# install dummykeyChain so performance test apps can link against it
cd /data/build/dummyKeychain
make install

#install standalone-pt app(s)
cd /data/build/tests
make install

cp -a /usr/bin/resources/ /home/joynr/resources

cd /home/joynr

ln -sf /usr/bin/performance-consumer-app-cc performance-consumer-app-cc

echo "/usr/lib" > /etc/ld.so.conf.d/usr-lib.conf