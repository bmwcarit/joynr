#!/bin/bash
if [ -d /data/scripts/extra-certs ]
then
    echo "Importing extra certs from /data/scripts/extra-certs"
    cd /data/scripts/extra-certs
    for file in *
    do
        echo "Importing $file"
        /usr/bin/keytool -importcert -file $file -alias $file -keystore /etc/pki/ca-trust/extracted/java/cacerts -storepass changeit -noprompt
    done
    echo "Import extra certs finished."
fi
exit 0
