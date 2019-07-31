#!/bin/bash
if [ -z "$PROXY_HOST_BUILD_ARG" ]
then
    echo "No proxy configured, using direct internet access."
    exit 0
fi
if [ -z "$PROXY_PORT_BUILD_ARG" ]
then
    echo "PROXY_PORT not set."
    exit 1
fi
proxy=""
if [ -z "$PROXY_USER_BUILD_ARG" ]
then
    proxy=http://$PROXY_HOST_BUILD_ARG:$PROXY_PORT_BUILD_ARG
else
    echo "PROXY_USER_BUILD_ARG set, using authenticated proxy."
    if [ -z "$PROXY_PASSWORD_BUILD_ARG" ]
    then
        echo "PROXY_USER_BUILD_ARG set but PROXY_PASSWORD_BUILD_ARG is not set."
        exit 1
    fi
    proxy=http://$PROXY_USER_BUILD_ARG:$PROXY_PASSWORD_BUILD_ARG@$PROXY_HOST_BUILD_ARG:$PROXY_PORT_BUILD_ARG
fi
echo "Starting to setup proxy configuration, PROXY_HOST_BUILD_ARG=$PROXY_HOST_BUILD_ARG, PROXY_HOST_BUILD_ARG=$PROXY_PORT_BUILD_ARG."
echo "Setting up proxy configuration in /etc/dnf/dnf.conf"
cat >> /etc/dnf/dnf.conf <<EOF
[main]
zchunk=false
gpgcheck=1
installonly_limit=3
clean_requirements_on_remove=false
proxy=$proxy
sslverify=false
EOF
echo "Final Configuration /etc/dnf/dnf.conf:"
cat /etc/dnf/dnf.conf
echo "Setting up proxy configuration in /etc/wgetrc"
cat > /etc/wgetrc <<EOF
use_proxy=on
http_proxy=$proxy
https_proxy=$proxy
ftp_proxy=$proxy
check_certificate=off
EOF
echo "Final Configuration /etc/wgetrc:"
cat /etc/wgetrc
echo "Setting up insecure curl configuration in /etc/.curlrc"
cat > /etc/.curlrc << EOF
insecure
EOF
echo "Setting up proxy configuration in /etc/profile.d/use-my-proxy.sh"
cat > /etc/profile.d/use-my-proxy.sh <<EOF
echo "use-my-proxy.sh started"
PROXY_HOST=$PROXY_HOST_BUILD_ARG
PROXY_PORT=$PROXY_PORT_BUILD_ARG
http_proxy="$proxy"
https_proxy="$proxy"
ftp_proxy="$proxy"
HTTP_PROXY="$proxy"
HTTPS_PROXY="$proxy"
FTP_PROXY="$proxy"
export PROXY_HOST
export PROXY_PORT
export http_proxy
export https_proxy
export ftp_proxy
export HTTP_PROXY
export HTTPS_PROXY
export FTP_PROXY
CURL_HOME=/etc
export CURL_HOME
echo "Setting up npm configuration in \$HOME/.npmrc"
cat > \$HOME/.npmrc << EOF2
strict-ssl=false
registry=http://registry.npmjs.org/
EOF2
MAVEN_OPTS="-Dmaven.wagon.http.ssl.insecure=true -Dmaven.wagon.http.ssl.allowall=true -Dmaven.wagon.http.ssl.ignore.validity.dates=true -Xms2048m -Xmx2048m"
export MAVEN_OPTS
echo "use-my-proxy.sh finished"
EOF
echo "Proxy configuration finished."
exit 0
