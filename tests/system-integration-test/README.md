# System Integration Tests
Please have a look into [docker/README.md](docker/README.md) for an overview about the System
Integration Test and instructions on how to run the tests.

# System Integration Tests - Incompatible fidl version tests

In addition to the regular SIT the apps from this folder can also be used for integration tests of the fidl version checking mechanism.
The steps needed for each language are described below.

There is a built-in mechanism in joynr which checks for compatible versions of the Franca interface definition between provider and consumer.
If an incompatibility has been detected, the consumer will raise an error and then exit.
This can be checked by running a provider and a consumer application which are running non-compatible fidl versions.

For the tests change the version of "src/main/model/SystemIntegrationTest.fidl"
======================================================================

```
package test

interface SystemIntegrationTest {
    version { major 1 minor 1 }   <------ Change major version here.
        method add {
            ...
        }
}
```

======================================================================

**Note: when adjusting the version numbers please consider the following:**
**compatible means: major version of provider == major version of proxy && minor version of provider >= minor version of proxy**

## C++

### Build the binaries

```
cd joynr/tests/system-integration-test/sit-cpp-app
rm -rf src/main/generated-sources/*
mkdir buildVersionOld
cd buildVersionOld
cmake ../ -DGENERATE_SOURCES:BOOL=ON
make -j4
cd ..
```

Increment the version number for 'major' in the fidl file (as shown above)

```
rm -rf src/main/generated-sources/*
mkdir buildVersionNew
cd buildVersionNew
cmake ../ -DGENERATE_SOURCES:BOOL=ON
make -j4
```

### Running the applications
Ensure you are running a cluster-controller that is accessible via WebSockets.

#### Test 1 - provider using a newer version of FIDL

```
buildVersionNew/bin/jsit-provider-ws io.joynr.systemintegrationtest.cpp
buildVersionOld/bin/jsit-consumer-ws io.joynr.systemintegrationtest.cpp
```

#### Test 2 - provider using an older version of FIDL

```
buildVersionOld/bin/jsit-provider-ws io.joynr.systemintegrationtest.cpp
buildVersionNew/bin/jsit-consumer-ws io.joynr.systemintegrationtest.cpp
```

### Expected result (for both tests)
This is the expected output from the consumer application.

```
terminate called after throwing an instance of 'joynr::exceptions::DiscoveryException'
  what():  Unable to find a provider with a compatible version. 1 incompabible versions found: Version{majorVersion:2 (in Test 2: 1), minorVersion:1}
Aborted
```

## Java

### Build the binaries

```
cd joynr/tests/system-integration-test/sit-java-app
mvn clean install
cp -R target/ targetVersionOld # Save current build
```

Increment the version numbers for 'major' in the fidl file (as shown above)

```
mvn clean install # re-run maven
cp -R target/ targetVersionNew
```

### Running the applications
Ensure you are running a cluster-controller that is accessible via WebSockets.
Use the following commands to start the provider and consumer:

```
java -cp sit-java-app-<joynr-version>-jar-with-dependencies.jar io.joynr.systemintegrationtest.ProviderApplication io.joynr.systemintegrationtest.node
java -cp sit-java-app-<joynr-version>-jar-with-dependencies.jar io.joynr.systemintegrationtest.ConsumerApplication io.joynr.systemintegrationtest.node
```

#### Test 1 - provider using a newer version of FIDL
Run the provider from the folder: targetVersionNew/ and run the consumer from the folder: targetVersionOld/

#### Test 2 - provider using an older version of FIDL
Run the provider from the folder: targetVersionOld/ and run the consumer from the folder: targetVersionNew/

### Expected result (for both tests)
result of failed version check:

```
...[ERROR] [] io.joynr.systemintegrationtest.ConsumerApplication: error creating proxy
```

## JavaScript

### Build the binaries

```
cd joynr/tests/system-integration-test/sit-node-app
mvn clean install
npm install
cp -R sit-node-app/ sit-node-app_OLD
```

Increment the version numbers for 'major' in the fidl file (as shown above)

```
mvn clean install
npm install
cp -R sit-node-app/ sit-node-app_NEW
```

### Running the applications
Ensure you are running a cluster-controller that is accessible via WebSockets.
Use the following commands to start the provider and consumer:

```
npm run-script startprovider
npm run-script startconsumer
```

#### Test 1 - provider using a newer version of FIDL
Run the provider from the folder: sit-node-app_NEW/ and run the consumer from the folder: sit-node-app_OLD/

#### Test 2 - provider using an older version of FIDL
Run the provider from the folder: sit-node-app_OLD/ and run the consumer from the folder: sit-node-app_NEW/

### Expected result (for both tests)
This is the expected output from the consumer application:

```
SIT RESULT error: node consumer -> io.joynr.systemintegrationtest.node error: {"_typeName":"joynr.exceptions.NoCompatibleProviderFoundException","detailMessage":"no compatible provider found within discovery timeout for domains \"[\"io.joynr.systemintegrationtest.node\"]\", interface \"test/SystemIntegrationTest\" with discoveryQos \"{\"discoveryTimeoutMs\":120000,\"discoveryRetryDelayMs\":10000,\"cacheMaxAgeMs\":0,\"discoveryScope\":{\"_typeName\":\"joynr.types.DiscoveryScope\",\"name\":\"LOCAL_THEN_GLOBAL\",\"value\":\"LOCAL_THEN_GLOBAL\"},\"providerMustSupportOnChange\":false,\"additionalParameters\":{}}\"","discoveredVersions":[{"_typeName":"joynr.types.Version","majorVersion":1,"minorVersion":1}],"interfaceName":"test/SystemIntegrationTest"}
```
