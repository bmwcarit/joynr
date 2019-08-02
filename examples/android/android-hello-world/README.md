# Android Examples

These Android Studio projects are basic examples of how to develop an Android joynr application.

Please note that MVVM paradigm should be used when making android applications, as is recomended  
by google and is followed in these examples.

# Android Hello World Consumer Example

This is a simple example of setting up a joynr consumer on Android, with a simple interface with a  
button to request a string from the provider. This example uses one joynr runtime and one cluster  
controller for access control, discovery and message routing.

# Android Hello World Provider Example

This is a simple example of setting up a joynr provider on Android, with a simple interface that  
logs the requests made. This example uses one joynr runtime and one cluster controller for access  
control, discovery and message routing.

## How To Build

Both projects are configured to build on android studio.

    Note: If the build process can't find the joynr classes you can run 
    the command "./gradlew joynrGenerate" in the command line.

## How to run

Both examples as is ready to run on google **AOSP** emulator.

In order to run any of the two examples you'll need an instance of a cluster controller running  
locally in your android emulator. If you want to, you can configure the example to connect to a  
remote cluster controller.

Also the examples are complementary so in order to run one of the examples you should also run  
the other one, or you can implement one. And the fidl files of both examples should be equals.