#!/bin/bash

echo "### Preparing HiveMQ broker configuration for TLS connections"

CERT_DIR='/data/ssl-data'
CERT_PATH=""
PRIVATE_KEY_PATH=""
KEYSTORE_PASSWORD='password'
CREATE_CERTS=0

REPO_DIR=`git rev-parse --show-toplevel 2>/dev/null`
if [ $? -ne 0 ]
then
    echo "### Command must be called from within repository directory"
    exit 1
fi

while getopts "cp:" OPTIONS;
do
    case $OPTIONS in
        c)  CREATE_CERTS=1
            ;;
        p)
            CERT_DIR=$OPTARG
            ;;
        \?)
            echo "### Synopsis: prepareBrokerConfigAndCerts.sh [-c} [-p <cert-path>]"
            echo "### This tool must be called from within a joynr repository clone."
            echo "### It creates a config.xml file for HiveMQ to allow TLS connections and"
            echo "### sets up key- and truststores with required certificates."
            echo "### The config file and the stores can then be mounted into docker"
            echo "### container for use by HiveMQ server."
            echo "### -c           optionally create new certificates"
            echo "### -p certdir   optionally specify certificate base directory path"
            echo "###              (default: /data/ssl-data),"
            exit 1
            ;;
    esac
done

if [ "$CREATE_CERTS" -eq 1 ]
then
    echo "### Creating certificates in $CERT_DIR"
    set -e
    mkdir -p $CERT_DIR
    $REPO_DIR/docker/joynr-base/scripts/docker/gen-certificates.sh --configfile $REPO_DIR/docker/joynr-base/openssl.conf --destdir $CERT_DIR
    set +e
fi

if [ ! -d "$CERT_DIR" ]
then
    echo "### Certificate base dir $CERT_DIR does not exist."
    exit 1
fi

CERT_PATH="$CERT_DIR/certs"
if [ ! -d "$CERT_PATH" ]
then
    echo "### Certificate path $CERT_PATH does not exist."
fi

PRIVATE_KEY_PATH="$CERT_DIR/private"
if [ ! -d "$PRIVATE_KEY_PATH" ]
then
    echo "### Private key path $PRIVATE_KEY_PATH does not exist."
fi

echo "### REPO_DIR=$REPO_DIR"
echo "### CERT_PATH=$CERT_PATH"
echo "### PRIVATE_KEY_PATH=$PRIVATE_KEY_PATH"

set -e

function createTrustStore
{
    # create JKS truststore
    echo "### Create JKS truststore"
    keytool -keystore catruststore.jks -importcert -file $CERT_PATH/ca.cert.pem -storepass $KEYSTORE_PASSWORD -trustcacerts -noprompt -storetype jks
    # list the JKS truststore contents
    echo "### List JKS truststore contents"
    keytool -list -keystore catruststore.jks -storepass $KEYSTORE_PASSWORD -storetype jks
}

function createCert
{
    CERT_NAME=$1
    # create new cert
    echo "### Create new ${CERT_NAME} certs"
    openssl req -nodes -keyout ${CERT_NAME}.key.pem -new -days 7300 -subj '/C=DE/ST=Bavaria/L=Munich/CN=localhost' -out ${CERT_NAME}.csr.pem -passout pass:password
    # sign cert
    echo "### Sign new ${CERT_NAME} certs"
    openssl x509 -req -days 7300 -CA ${CERT_PATH}/ca.cert.pem -CAkey ${PRIVATE_KEY_PATH}/ca.key.pem -set_serial 01 -in ${CERT_NAME}.csr.pem -out ${CERT_NAME}.cert.pem
    # import cert into P12 keystore
    echo "### Import ${CERT_NAME} certs into ${CERT_NAME}keystore.jks"
    openssl pkcs12 -export -in ${CERT_NAME}.cert.pem -inkey ${CERT_NAME}.key.pem -out ${CERT_NAME}keystore.jks -password pass:password
    # convert P12 keystore into JKS keystore
    keytool -importkeystore -deststorepass password -destkeypass password -destkeystore ${CERT_NAME}keystore.jks -srckeystore ${CERT_NAME}keystore.jks -srcstoretype PKCS12 -srcstorepass password -alias 1 -storepass password -storetype jks
    # list JKS keystore contents
    echo "### List keystore ${CERT_NAME}keystore.jks"
    keytool -list -keystore ${CERT_NAME}keystore.jks -storepass password -storetype jks
}

cd $REPO_DIR/docker/joynr-mqttbroker
rm -fr var
mkdir var
cd var
cp ../config/config.xml .
mkdir certs
cd certs

createTrustStore
createCert "server"
createCert "client"

rm -f *.old *.pem
chmod a+r *
exit 0
