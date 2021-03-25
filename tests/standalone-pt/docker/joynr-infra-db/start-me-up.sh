#!/bin/bash

asadmin --user admin --passwordfile=/opt/payara41/pwdfile start-database --jvmoptions="-Dderby.storage.useDefaultFilePermissions=true"
asadmin --user admin --passwordfile=/opt/payara41/pwdfile start-domain --verbose

echo "end of start-me-up of joynr-infra-db"
