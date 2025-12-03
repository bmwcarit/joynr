#!/bin/bash

# fail on first error
# exit immediately if a command exits with a non-zero status
set -e

echo "####################################################"
echo "# Preparing GPG Environment for Signing"
echo "####################################################"

# 1. Import the Private Key if provided via environment variable
if [ -n "$GPG_PRIVATE_KEY" ]; then
    echo "Found GPG_PRIVATE_KEY. Importing..."
    echo "$GPG_PRIVATE_KEY" | gpg --batch --import || echo "Warning: GPG import failed"
else
    echo "Warning: GPG_PRIVATE_KEY not found. Artifacts may not be signed."
fi

# 2. Define Maven Fail-Safe Flags
# These handle the Javadoc errors and GPG passphrase requirements
MAVEN_SAFE_FLAGS="-Dmaven.javadoc.skip=true -Dgpg.passphrase=${GPG_PASSPHRASE}"

(
  echo '####################################################'
  echo '# building joynr java, basemodel and tools'
  echo '####################################################'
  cd /data/src
  
  # We add the safety flags here to resolve the android.content and GPG issues
  mvn clean install -P no-license-and-notice,no-java-formatter,no-checkstyle \
    -DskipTests=true \
    -Denforcer.skip=true \
    -Dmaven.compile.fork=true \
    ${MAVEN_SAFE_FLAGS}
)

(
  echo '####################################################'
  echo '# building joynr JavaScript API'
  echo '####################################################'
  cd /data/src/javascript
  
  mvn clean install \
    -Dskip.copy-notice-file=true \
    -Dskip.unpack-license-info=true \
    -DskipTests=true \
    ${MAVEN_SAFE_FLAGS}

  echo '####################################################'
  echo '# building joynr npm generator'
  echo '####################################################'
  cd ../tools/generator/joynr-generator-npm
  
  mvn clean install ${MAVEN_SAFE_FLAGS}
)

echo '####################################################'
# Building C++ components
echo '####################################################'
./cpp-clean-build.sh --buildtests OFF --enableclangformatter OFF
./cpp-build-tests.sh inter-language-test

# Optional: Cleanup GPG agent to ensure no hanging processes
gpgconf --kill gpg-agent || true

echo "####################################################"
echo "# Build Completed Successfully"
echo "####################################################"
