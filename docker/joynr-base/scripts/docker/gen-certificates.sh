#!/bin/bash

DEST_DIR='.'
CONFIG_FILE=""

function usage
{
    echo "usage: gen-certificates.sh --configfile <filename> [--destdir <dir>]"
}

while [ "$1" != "" ]; do
    case $1 in
        --configfile )           shift
                                 CONFIG_FILE=$1
                                 ;;

        --destdir )              shift
                                 DEST_DIR=${1%/}/
                                 ;;
        * )                      usage
                                 exit 1
    esac
    shift
done

if [ "$CONFIG_FILE" == "" ]; then
    echo "No config file specified ..."
    echo " "
    usage
    exit -1
fi


if [ "$DEST_DIR" == "" ]; then
    echo "No destination directory specified. Using current directory"
fi

cd $DEST_DIR
(
mkdir -p private
mkdir -p certs

openssl req -nodes -config $CONFIG_FILE -subj '/CN=ca' -keyout private/ca.key.pem -new -x509 -days 7300 -sha256 -extensions v3_ca -out certs/ca.cert.pem
openssl req -nodes -keyout private/server.key.pem -new -days 7300 -subj '/C=DE/ST=Bavaria/L=Munich/CN=localhost' -out certs/server.csr.pem
openssl x509 -req -days 7300 -CA certs/ca.cert.pem -CAkey private/ca.key.pem -set_serial 01 -in certs/server.csr.pem -out certs/server.cert.pem
openssl req -nodes -keyout private/client.key.pem -new -days 7300 -subj '/C=DE/ST=Bavaria/L=Munich/CN=client' -out certs/client.csr.pem
openssl x509 -req -days 7300 -CA certs/ca.cert.pem -CAkey private/ca.key.pem -set_serial 01 -in certs/client.csr.pem -out certs/client.cert.pem
chmod 644 certs/*.pem private/*.pem
)
