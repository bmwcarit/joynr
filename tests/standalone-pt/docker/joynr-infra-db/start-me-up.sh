#!/bin/bash

asadmin start-database --jvmoptions="-Dderby.storage.useDefaultFilePermissions=true"
asadmin start-domain --verbose

echo "end of start-me-up of joynr-infra-db"
