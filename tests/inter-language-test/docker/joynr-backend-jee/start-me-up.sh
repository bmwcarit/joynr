#!/bin/bash

asadmin start-database --jvmoptions="-Dderby.storage.useDefaultFilePermissions=true"
asadmin start-domain --verbose
