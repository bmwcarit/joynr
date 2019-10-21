# Binder for joynr

## Introduction

Joynr for Android uses Binder as a Transport Layer, replacing websockets from the Java  
implementation. This is done in order to take full advantage of the Android framework, and use its  
native way of performing inter-process communication (IPC).

## Structure

Joynr has 3 types of nodes:
- Cluster Controller; Consumer; Provider

But our Binder implementation is agnostic. Therefore, it doesn't matter where it is running, as the  
only goal of the transport layer is to send and receive messages between processes. All nodes act as  
both Clients or Servers. 
A Client is defined as the node that sends a message, and a Server is defined as the node that  
receives the message.
Wakeup on demand is a given, since bindService wakes up the Service when binding even if it is not  
running.

## Architecture

Sending a message

```
Runtime         Client          Stub                       Skeleton          Server         Runtime
  |               |               |                            |               |               |
  |---transmit--->|---transmit--->|-------------bind---------->|---transmit--->|---transmit--->|
```

Message-Response workflow

```
CC Runtime    Service     Stub     Skeleton      Skeleton     Stub      Service    Libjoynr Runtime
     |          |          |          |             |          |           |              |
     |<------------transmit-----------|<----------bind---------|<-------transmit----------|
     |          |          |          |             |          |           |              |
     |-------transmit----->|---------bind---------->|-------------transmit--------------->|

```
Joynr uses a RMI based Stub-Skeleton structure. 
There is a Stub in the Client side, which acts as a proxy for the remote service. It is responsible  
for the serialization of the request objects and implements the logic to send it to the server.
In the Server side there is a skeleton, which is responsible for deserializing the request and  
routing it to the service implementation.

Binder works as the transport layer with a Client-Server structure, where:
- the Server is an Android Service that receives messages from the skeleton and then redirects them  
to the Routing table to be routed to its intended recipient through the transmit() function.
- the Client uses context.bindService(...) to connect to the Server. The binding is done using the  
package name and service name of the Server, stored in a BinderAddress object.
- Libjoynr will use the Client when it needs to send a message, and route it through the Stub.
- the Server side implements an AIDL interface that has only 1 function (transmit) responsible for  
sending messages to the Server, using the binder object obtained with the ServiceConnection. The  
Server then redirects the message.
- Communication is one-way only. Only the Client sends information to the Server, and the Server  
does not respond back. If there is a response to a message, the process is the same, but reversed.  
The node that received the previous message is now the client and needs to communicate with the  
other node which has a Service that also listens for messages. 

## Runtime

A specific Runtime was created for Android, which allows developers to initialize it without much  
configuration.

```
AndroidBinderRuntime.initClusterController(context, brokerUri, properties);
```

for initializing the Cluster Controller Runtime, or

```
AndroidBinderRuntime.init(context); or AndroidBinderRuntime.init(context, properties); 
```

for initializing the Libjoynr Runtime (for Consumers and Providers)

In order to Libjoynr runtime communicate with Cluster controller runtime it's necessary to have the
cluster controller app installed. This is necessary because Libjoynr runtime automatically queries 
the Android system using the PackageManager for an Android service that has an intent-filter with 
the following action: 

``` 
io.joynr.android.action.COMMUNICATE
```

There is one example of this mechanism in Android Cluster Controller Standalone project 
and joynr developers only need to worry about this mechanism in case they are responsible for 
joynr cluster controller implementation. In case the Libjoynr runtime is initialized and there is 
no joynr cluster controller app installed, a JoynrRuntimeException is thrown alerting the developer.

## BinderAddress

A new type of Routing Address was added to enable communication between 2 processes in different  
packages in Android.
BinderAddress has 1 field:
- packageName

## Packages

The added package for Binder to work on Android is:
- joynr-android-binder-runtime

## Development

There is a boolean variable(devEnv) in gradle.properties file that allows developers to use the  
sources instead of the binaries defined in build.gradle file. This is useful for debugging and 
instant development purposes.

## Known Issues

- Binder runs on the main (UI) thread as well as the the ServiceConnection object that receives the  
binder object (Stub) from the Server. The usage of Futures to get a response in the UI thread blocks  
it. If the ServiceConnection object has not received the Binder object, this will result in a deadlock..
So, to obtain results from Futures, as developer should perform the future.get() inside a new  
Thread, e.g.:

```
Future<Void> future = runtime.getProviderRegistrar("domain", provider)
        .awaitGlobalRegistration()
        .withProviderQos(providerQos)
        .register();

new Thread(new Runnable() {
    @Override
    public void run() {
        try {
            future.get();
        } catch (InterruptedException e) {
            e.printStackTrace();
        } catch (ApplicationException e) {
            e.printStackTrace();
        }
    }
});
```
