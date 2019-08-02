### What is this and why it exists

This is libjoynr's Android WebSocket runtime for Android applications. It corresponds to its Java  
counterpart and should simplify joynr Android development.

### Relevant changes from Java version
 
#### Guice without AOP

We bundle the Java library dependencies but exclude the AOP from Google's Guice dependency  
framework.

The Guice AOP version [is not supported in Android](https://github.com/google/guice/wiki/AOP#limitations)  
and thus it's necessary to configure joynr libraries to use the provided **no_aop** Guice version.

### How to compile and publish it under local maven repository

<pre> gradle publishToMavenLocal </pre>

### How to use it

Add this line to build.gradle file and verify if you have mavenLocal() under your maven   
repositories list.

<pre> implementation ('io.joynr.android:libjoynr-android-websocket-runtime:1.11.0-SNAPSHOT')</pre>