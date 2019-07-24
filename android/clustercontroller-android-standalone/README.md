## Joynr Cluster Controller Standalone

This project creates an Android application(apk) that can be used in any Android device/Emulator
running an Android virtual machine (dalvik/art) and includes the Joynr Cluster Controller.

The cluster controller is configured to start listening websocket communication locally using the 
**localhost** under port **4242**.

The mqtt broker endpoint needs to be passed using two different possible ways:
- When the app is used in dev environment can be configured in the UI after starting the app;
- When the app is started by CI or command line environment the argument can be passed in the intent
that starts the activity passing the following extra (EXTRA_BROKER_URI) in this way:
`  $ adb shell am start -n io.joynr.android.clustercontrollerstandalone/.MainActivity  \ 
-e "EXTRA_BROKER_URI" "tcp://brokerip:brokerport"`

**Attention**: the `brokerip` and `brokerport` needs to be changed.

The app starts a foreground service constantly running in the background and creates an ongoing
notification so that the foreground service can be running in the background. When the user clicks 
in the ongoing notification the activity is brought to the front and the user can stop the cluster 
controller.  