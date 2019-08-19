# joynr Cluster Controller Standalone

This project creates an Android application (APK) that can be used in any Android device/emulator
running an Android virtual machine (dalvik/art) and includes the joynr Cluster Controller.

The cluster controller is configured to start listening to WebSocket communication locally using
the **localhost** under port **4242**.

The MQTT broker endpoint needs to be passed using one of two possible ways:

- The app is used in the dev environment -- MQTT broker endpoint is to be configured in the UI
  after starting the app;

- The app is started by CI or via command line interface -- MQTT broker endpoint is to be passed
  via an intent that starts the activity passing the following extra (EXTRA_BROKER_URI) in this
  way:

`$ adb shell am start -n io.joynr.android.clustercontrollerstandalone/.MainActivity  \ -e
"EXTRA_BROKER_URI" "tcp://brokerip:brokerport"`

**Attention**: the `brokerip` and `brokerport` need to be changed.

The app starts a Android foreground service which is constantly running. This service creates an
ongoing notification so that the foreground service can be constantly running. When the user clicks
in the ongoing notification the activity is brought to the front and the user can stop the cluster
controller.
