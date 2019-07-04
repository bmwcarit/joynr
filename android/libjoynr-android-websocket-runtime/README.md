### What is this and why it exists

This is a bundle of libraries reconfigured to do not depend on aop google guice dependency framework.
The guice aop version is not supported on [Android]( https://github.com/google/guice/wiki/AOP#limitations) and it's necessary to configure joynr libraries to use the **no_aop** guice version.
It's possible to implement the workarounds implemented in this project under the app's sources however by implementing this we remove this impediment/blocking situation from the android developers and allow them to focus on their internal development tasks.


### How to compile and publish it under local maven repository

<pre> gradle publishToMavenLocal </pre>

### How to use it

Add this line to build.gradle file and verify if you have mavenLocal() under your maven repositories list.

<pre> implementation ('io.joynr.android:joynr-android:0.1') </pre>