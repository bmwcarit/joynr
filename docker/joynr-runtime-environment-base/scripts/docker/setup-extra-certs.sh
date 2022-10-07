#!/bin/bash
if [ -d /data/scripts/extra-certs ]
then
    echo "Importing extra certs from /data/scripts/extra-certs"
    cd /data/scripts/extra-certs
    for file in *
    do
        if [ "$1" == "java" ]
        then
            echo "Importing $file to Java keystore."
            /usr/bin/keytool -importcert -file $file -alias $file -keystore /etc/pki/ca-trust/extracted/java/cacerts -storepass changeit -noprompt
        else
            echo "Copying $file to /etc/pki/ca-trust/source/anchors"
            cp $file /etc/pki/ca-trust/source/anchors
        fi
    done
    if [ "$1" != "java" ]
    then
        echo "Running update-ca-trust."
        update-ca-trust
    fi
    echo "Importing extra certs finished."
else
    echo "No extra certs specified."
fi
exit 0
