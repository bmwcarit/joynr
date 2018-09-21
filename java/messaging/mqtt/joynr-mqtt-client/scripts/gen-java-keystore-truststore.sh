#!/bin/bash

DEST_DIR='.'
KEYSTORE_PASSWORD='password'

# these files are located inside the docker image 
CERT_PATH='/data/ssl-data/certs'
PRIVATE_KEY_PATH='/data/ssl-data/private'

function usage
{
    echo "usage: gen-java-keystore-truststore.sh
	[--keystorepassword <non empty password for the keystore, default: 'password'>]
	[--destdir <existing dir, default: '.'>]"
}

while [ "$1" != "" ]; do
    case $1 in
	
	--keystorepassword )     shift
                                 KEYSTORE_PASSWORD=$1
                                 ;;

	--destdir )              shift
                                 DEST_DIR=${1%/}/
                                 ;;

        * )                      usage
                                 exit 1
    esac
    shift
done

if [ -z "$KEYSTORE_PASSWORD" ]; then
    echo "Empty password for the keystore specified ..."
    echo " "
    usage
    exit -1
fi


if [ -z "$DEST_DIR" ]; then
    echo "No destination directory specified. Using current directory"
fi


cd "$DEST_DIR"


# create JKS truststore
keytool -keystore catruststore.jks -importcert -file $CERT_PATH/ca.cert.pem -storepass $KEYSTORE_PASSWORD -trustcacerts -noprompt

# list the truststore contents
keytool -list -keystore catruststore.jks -storepass $KEYSTORE_PASSWORD

# create PKCS12 truststore
keytool -importkeystore -srckeystore catruststore.jks -srcstorepass $KEYSTORE_PASSWORD -destkeystore catruststore.p12 -deststorepass $KEYSTORE_PASSWORD -srcstoretype JKS -deststoretype PKCS12  -noprompt

# merge and import client certificate and private key into pkcs12 keystore
openssl pkcs12 -export -in $CERT_PATH/client.cert.pem -inkey $PRIVATE_KEY_PATH/client.key.pem -out clientkeystore.p12 -password pass:$KEYSTORE_PASSWORD

# convert pkcs12 keystore into java keystore
keytool -delete -importkeystore -deststorepass $KEYSTORE_PASSWORD -destkeypass $KEYSTORE_PASSWORD -destkeystore clientkeystore.jks -srckeystore clientkeystore.p12 -srcstoretype PKCS12 -srcstorepass $KEYSTORE_PASSWORD -alias 1 -storepass $KEYSTORE_PASSWORD -noprompt

# list the keystore contents
keytool -list -keystore clientkeystore.jks -storepass $KEYSTORE_PASSWORD
