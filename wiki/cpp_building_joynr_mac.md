# Building joynr C++ in Mac OS X
It is possible to build joynr C++ natively on Mac OS X, but you have to
install some extra software and tweak some scripts to get it to work.

This short How-To gives you a step-by-step guide of getting everything
set-up and building.

The instruction assumes Mac OS X Sierra 10.12.1.

## Prerequisites
### Environment Setup
* Install [XCode](https://developer.apple.com/xcode/) or from Apple Store
* Install [Homebrew](http://brew.sh/)
  * Install boost - brew install boost
  * Install cmake - brew install cmake
  * Install openssl - brew install openssl
  * Install mosquitto - brew install mosquitto

### Fetch required modules and generate sources
```bash
mvn clean install -P no-license-and-notice,no-java-formatter,no-checkstyle -DskipTests
```

## Create and link the build script
1.  Create a script (e.g. `build-cpp.sh`) which will execute the build.
This can be placed outside of the joynr directory.

```bash
#!/bin/bash

cmake -DUSE_PLATFORM_MOSQUITTO=ON \
  -DOPENSSL_ROOT_DIR=[YOUR OPEN SSL ROOT DIR] \
  -DOPENSSL_LIBRARIES=[YOUR OPEN SSL LIB DIR] \
  -DENABLE_GCOV=OFF \
  -DENABLE_DOXYGEN=OFF \
  -DUSE_PLATFORM_SPDLOG=OFF \
  -DUSE_PLATFORM_MUESLI=OFF \
  -DUSE_PLATFORM_WEBSOCKETPP=OFF \
  -DENABLE_CLANG_FORMATTER=OFF \
  -DBUILD_TESTS=OFF \
  -DJOYNR_SERVER_HOST=localhost \
  -DJOYNR_SERVER_MQTT_PORT=1883 \
  -DCMAKE_BUILD_TYPE=Debug ../cpp

make -j 8
```

  * The OpenSSL version provided by Mac OS X is not compatible with joynr, therefor it is necessary
  to supply the OpenSSL version installed using Homebrew to CMake:


```bash
  -DOPENSSL_ROOT_DIR=[YOUR OPEN SSL ROOT DIR]/usr/local/Cellar/openssl/1.0.2j/ \
  -DOPENSSL_LIBRARIES=[YOUR OPEN SSL LIB DIR]/usr/local/Cellar/openssl/1.0.2j/lib \
```

2. Change into your joynr workspace home (e.g. `~/projects/joynr`) and create a
separate build directory called `cpp-build`. Change into this, and symlink
the script created in the above step into this dir
(e.g. `ln -s ../../build-cpp.sh .`).


## Compiling with gcc

If you have installed gcc on your mac, then the cmake command will look like the one below:

```bash
#!/bin/bash

cmake -DCMAKE_C_COMPILER=/usr/local/gcc-6.2.0/bin/gcc-6.2.0 \
  -DCMAKE_CXX_COMPILER=/usr/local/gcc-6.2.0/bin/g++-6.2.0 \
  -DCMAKE_PREFIX_PATH:UNINITIALIZED=/usr/local/gcc-6.2.0 \
  -DUSE_PLATFORM_MOSQUITTO=ON \
  -DOPENSSL_ROOT_DIR=/usr/local/Cellar/openssl/1.0.2j/ \
  -DOPENSSL_LIBRARIES=/usr/local/Cellar/openssl/1.0.2j/lib \
  -DENABLE_GCOV=OFF \
  -DENABLE_DOXYGEN=OFF \
  -DUSE_PLATFORM_SPDLOG=OFF \
  -DUSE_PLATFORM_MUESLI=OFF \
  -DUSE_PLATFORM_WEBSOCKETPP=OFF \
  -DENABLE_CLANG_FORMATTER=OFF \
  -DBUILD_TESTS=OFF \
  -DJOYNR_SERVER_HOST=localhost \
  -DJOYNR_SERVER_MQTT_PORT=1883 \
  -DCMAKE_BUILD_TYPE=Debug ../cpp

make -j 8
```
