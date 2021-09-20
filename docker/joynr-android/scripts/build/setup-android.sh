#!/bin/bash
echo "Starting setup-android.sh"
. /etc/profile
if [ -z "${ANDROID_HOME}" ]
then
	echo "ANDROID_HOME not set."
	exit 1
fi
if [ -z "${ANDROID_API_LEVEL}" ]
then
	echo "ANDROID_API_LEVEL not set."
	exit 1
fi
if [ -z "${ANDROID_BUILD_TOOLS_VERSION}" ]
then
	echo "ANDROID_BUILD_TOOLS_VERSION not set."
	exit 1
fi

set -e
SDK_MANAGER=${ANDROID_HOME}/cmdline-tools/latest/bin/sdkmanager
PROXY_ARGS=""
if [ -n "$PROXY_HOST" ]
then
	PROXY_ARGS="--no_https --proxy=http --proxy_host=$PROXY_HOST --proxy_port=$PROXY_PORT"
fi
cd ${ANDROID_HOME}
echo y | ${SDK_MANAGER} --install --verbose $PROXY_ARGS "platforms;${ANDROID_API_LEVEL}"
echo y | ${SDK_MANAGER} --install --verbose $PROXY_ARGS "build-tools;${ANDROID_BUILD_TOOLS_VERSION}"
echo y | ${SDK_MANAGER} --install --verbose $PROXY_ARGS "platform-tools"
echo y | ${SDK_MANAGER} --install --verbose $PROXY_ARGS "system-images;${ANDROID_API_LEVEL};default;x86_64"
echo y | ${SDK_MANAGER} --install --verbose $PROXY_ARGS emulator

echo "Finished setup-android.sh"
