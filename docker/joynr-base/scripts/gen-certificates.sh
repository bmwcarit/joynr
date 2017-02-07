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

openssl req -nodes -config $CONFIG_FILE -subj '/CN=test' -keyout private/ca.key.pem -new -x509 -days 7300 -sha256 -extensions v3_ca -out certs/ca.cert.pem
openssl req -nodes -keyout private/cc.key.pem -new -days 7300 -subj '/C=DE/ST=Bavaria/L=Munich/CN=cc' -out certs/cc.csr.pem
openssl x509 -req -days 7300 -CA certs/ca.cert.pem -CAkey private/ca.key.pem -set_serial 01 -in certs/cc.csr.pem -out certs/cc.cert.pem
openssl req -nodes -keyout private/libjoynr.key.pem -new -days 7300 -subj '/C=DE/ST=Bavaria/L=Munich/CN=cc' -out certs/libjoynr.csr.pem
openssl x509 -req -days 7300 -CA certs/ca.cert.pem -CAkey private/ca.key.pem -set_serial 01 -in certs/libjoynr.csr.pem -out certs/libjoynr.cert.pem
)
