# What is this and why it exists

This is libjoynr's Android WebSocket runtime for Android applications. It corresponds to its Java  
counterpart and should simplify joynr Android development.

## Relevant changes from Java version

### Guice without AOP

We bundle the Java library dependencies but exclude the AOP from Google's Guice dependency  
framework.

The Guice AOP version [is not supported in
Android](https://github.com/google/guice/wiki/AOP#limitations)  
and thus it's necessary to configure joynr libraries to use the provided **no_aop** Guice version.

## How to compile and publish it under local maven repository

`gradle publishToMavenLocal`

## How to use it

Add this line to build.gradle file and verify if you have mavenLocal() under your maven
repositories list.

`implementation 'io.joynr.android:libjoynr-android-websocket-runtime:1.19.7'`

## Logging

It may not be desirable to have the various Android joynr components log all their information to
logcat while developing certain applications. Thus, we have included a way for logging to be done
in the joynr toolchain.

We have created our own version of the SLF4J implementation so that the log level can be
controlled. Depending on the log level, the component using this implementation will log
accordingly. libjoynr for Android depends on this implementation, which exposes it to all libjoynr
users.

Internally, libjoynr implements an Android `BroadcastReceiver` that will automatically apply the
log level that is passed in the `Intent`. Cluster Controller depends on libjoynr, so this strategy
can also be used to control its logging output. All of this happens at runtime.

### How To Use

Assuming that ADB is available in the machine and is configured in the PATH environment variable,
logging can be controlled by sending a broadcast through the command line:

`adb shell am broadcast -a io.joynr.android.LOG_LEVEL_CHANGE -n
{app-package-name}/.receivers.PropertyChangeReceiver -e setlevel "{LEVEL}"`

**{app-package-name}** should be the app package that will have its logging level adapted.

**{LEVEL}** can be any of the following, based on SLF4J's log levels:

- VERBOSE (the least serious -- more verbose)
- DEBUG
- INFO
- WARN
- ERROR
- OFF (no logging)

Choosing a log level will allow for logging of all levels with higher severity. For example, INFO
will also log output of WARN and ERROR, and VERBOSE will log evrything.

The default level for logging (i.e., without configuring) is set to WARN. Broadcasting an
unsupported level will also fall back to WARN.
