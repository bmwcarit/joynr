#!/bin/bash
if [ -d /data/scripts/extra-certs ]
then
    echo "Importing extra certs from /data/scripts/extra-certs"
    cd /data/scripts/extra-certs
    for file in *
    do
        echo "Importing $file"
        /usr/bin/keytool -importcert -file $file -alias $file -keystore /etc/ssl/certs/java/cacerts -storepass changeit -noprompt
        cp $file /usr/local/share/ca-certificates
    done
    update-ca-certificates 2>/dev/null
    echo "Import extra certs finished."

fi
exit 0
