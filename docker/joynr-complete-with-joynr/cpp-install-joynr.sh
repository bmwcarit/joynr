#!/bin/bash
set -x
. /etc/profile
mkdir -p /root/.m2
if [ -f /data/src-helper/settings.xml ]
then
    echo "Using provided settings.xml"
    cp /data/src-helper/settings.xml /root/.m2
else
    if [ -n "$PROXY_HOST" ]
    then
        echo "Using provided settings.xml.prototype"
        cp /data/src-helper/settings.xml.prototype /root/.m2/settings.xml
        sed -i "s/PROXY_HOST/${PROXY_HOST}/" /root/.m2/settings.xml
        sed -i "s/PROXY_PORT/${PROXY_PORT}/" /root/.m2/settings.xml
        echo "--- Resulting /root/.m2/settings.xml ---"
        cat /root/.m2/settings.xml
        echo "--- Resulting /root/.m2/settings.xml ---"
    fi
fi
echo "BEGIN Listing $HOME/.m2"
ls -al $HOME/.m2
echo "END Listing $HOME/.m2"
cat > /root/.mavenrc << EOF
export JAVA_HOME=/usr/lib/jvm/java-openjdk
EOF
/data/src/docker/joynr-ilt-gcc/scripts/build/cpp-generate.sh
/data/src/docker/joynr-ilt-gcc/scripts/build/cpp-clean-build.sh --jobs 16 --buildtests OFF --enableclangformatter OFF
cd /data/build/joynr
make install
echo "/usr/lib" > /etc/ld.so.conf.d/usr-lib.conf
