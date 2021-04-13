#!/bin/bash
if [ -n "$http_proxy" ]; then
    if [[ "$http_proxy" =~ .*"@" ]]; then
        read protocol proxy_user proxy_password proxy_host proxy_port <<< $( echo ${http_proxy} | awk -F '://|:|@' '{ print $1, $2, $3, $4, $5 }')
    else
        read protocol proxy_host proxy_port <<< $( echo ${http_proxy} | awk -F'://|:' '{ print $1, $2, $3 }')
    fi
    echo "protocol = $protocol"
    echo "proxy_host = $proxy_host"
    echo "proxy_port = $proxy_port"
    echo "proxy_user = $proxy_user"
    echo "proxy_password = $proxy_password"

    proxy_url=${protocol}://${proxy_host}:${proxy_port}
fi

if [ -z "$proxy_host" ]
then
    echo "No proxy configured, using direct internet access."
    exit 0
else
    if [ -z "$proxy_port" ]
    then
        echo "No port for proxy defined."
        exit 1
    fi
    if [ -n "$proxy_user" ]
    then
        echo "Using authenticated proxy."
        if [ -z "$proxy_password" ]
        then
            echo "Proxy user is set but no proxy password supplied."
            exit 1
        fi
    else
        echo "Using Proxy."
    fi
fi
echo "Setting up proxy configuration in /etc/dnf/dnf.conf"
cat >> /etc/dnf/dnf.conf <<EOF
zchunk=false
gpgcheck=1
installonly_limit=3
clean_requirements_on_remove=false
sslverify=false
EOF
# add dnf proxy configuration if available
if [ -n "${http_proxy}" ]; then
  cat >> /etc/dnf/dnf.conf <<EOF
proxy=${proxy_url}
EOF
  # add user/password for proxy if available
  if [ -n "${proxy_user}" ]; then
    cat >> /etc/dnf/dnf.conf <<EOF
proxy_username=${proxy_user}
proxy_password=${proxy_password}
EOF
  fi
fi
echo "Final Configuration /etc/dnf/dnf.conf:"
cat /etc/dnf/dnf.conf
echo "Setting up proxy configuration in /etc/wgetrc"
cat > /etc/wgetrc <<EOF
check_certificate=off
EOF
# add wget proxy configuration if available
if [ -n "${http_proxy}" ]; then
  cat >> /etc/wgetrc <<EOF
use_proxy=on
http_proxy=${http_proxy}
https_proxy=${http_proxy}
ftp_proxy=${http_proxy}
EOF
fi
echo "Final Configuration /etc/wgetrc:"
cat /etc/wgetrc
echo "Setting up insecure curl configuration in /etc/.curlrc"
cat > /etc/.curlrc << EOF
insecure
EOF
echo "Setting up proxy configuration in /etc/profile.d/use-my-proxy.sh"
cat > /etc/profile.d/use-my-proxy.sh <<EOF
echo "use-my-proxy.sh started"
PROXY_HOST=$proxy_host
PROXY_PORT=$proxy_port
http_proxy="$http_proxy"
https_proxy="$http_proxy"
ftp_proxy="$http_proxy"
HTTP_PROXY="$http_proxy"
HTTPS_PROXY="$http_proxy"
FTP_PROXY="$http_proxy"
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
GRADLE_CONFIG_DIR=/home/joynr/.gradle
echo "Setting up gradle configuration in $GRADLE_CONFIG_DIR"
mkdir $GRADLE_CONFIG_DIR
cat > $GRADLE_CONFIG_DIR/gradle.properties <<EOF
systemProp.http.proxyHost=$proxy_host
systemProp.http.proxyPort=$proxy_port
systemProp.https.proxyHost=$proxy_host
systemProp.https.proxyPort=$proxy_port
EOF
if [ -n "$proxy_user" ]
then
cat >> $GRADLE_CONFIG_DIR/gradle.properties <<EOF
systemProp.http.proxyUser=$proxy_user
systemProp.http.proxyPassword=$proxy_password
systemProp.https.proxyUser=$proxy_user
systemProp.https.proxyPassword=$proxy_password
EOF
fi
chmod a+rx $GRADLE_CONFIG_DIR
chmod a+r $GRADLE_CONFIG_DIR/gradle.properties
exit 0
