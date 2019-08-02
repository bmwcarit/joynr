#!/bin/sh
set -eux

#REMOVE THIS AFTER NESTED VIRTUALZATION IS READY
echo "Exiting due to no nested virtualization available yet!"
exit 0

#create android virtual device (avd)
echo no | /opt/android-sdk-linux/tools/bin/avdmanager create avd -n joynr -k "system-images;android-28;default;x86_64" --force

#start the avd
/opt/android-sdk-linux/emulator/emulator-headless -avd joynr -no-window &
AVD_PID=$!

echo -e "\n\n[INFO] Waiting for android virtual device\n"
adb wait-for-device shell 'while [[ -z $(getprop sys.boot_completed) ]]; do sleep 1; done;'

echo -e "\n\n[INFO] Compiling, installing and starting android cluster controller...\n"
../clustercontroller-android-standalone/gradlew installDebug -p ../clustercontroller-android-standalone/
adb shell am start -n io.joynr.android.clustercontrollerstandalone/.MainActivity -e "EXTRA_BROKER_URI" "tcp://10.0.2.2:1883"

echo -e "\n\n[INFO] Compiling, installing and starting test radio provider...\n"
./test-radio-provider/gradlew installDebug -p ./test-radio-provider
adb shell am start -n "io.joynr.android.provider/io.joynr.android.provider.MainActivity" -a android.intent.action.MAIN -c android.intent.category.LAUNCHER

echo -e "\n\n[INFO] Compiling and running test radio consumer tests...\n"
./test-radio-consumer/gradlew connectedDebugAndroidTest -p ./test-radio-consumer/

#terminate avd
kill -9 $AVD_PID
