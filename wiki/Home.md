# Joynr
## What is joynr?

joynr is a fault-tolerant, typed communications-middleware abstraction framework for applications and services deployed to vehicles, consumer devices and backend servers that need to interact with each other.

joynr simplifies data access and data exchange by abstracting the pub/sub, RPC and broadcast/event paradigms found in many modern middleware solutions. This abstraction allows the use of a range of technical middleware solutions, depending on the needs of the deployment.

## Key benefits

### Formally-specified, typed interfaces

* joynr helps prevent programming errors related to interface incompatibility by
having interfaces modelled in a special Interface Definition Language (IDL).

* The data exchanged is also typed, so that there is no confusion about what has been received.

* Formally-specified interfaces also provide the ability to automate testing, generate documentation etc.

* joynr uses Franca IDL as an open-source industry standard that has already been adopted by the automobile industry for use with the likewise open-source C++ API CommonAPI. This allows for a high degree of interoperability between CommonAPI and joynr.

### Logical Peer-to-Peer Networking

* joynr enables network topologies that cannot (for technical or other reasons) be spanned by a single
transport middleware.

* For example, using joynr:

 * Web Sockets may be used between devices
 * MQTT or HTTP Long Polling may be used to the backend

* Since joynr is transport middleware agnostic, operations may swap out middleware implementations and infrastructure without requiring application rewrites. For example, a joynr network could replace HTTP Long Polling with Web Sockets without requiring the applications using joynr to be modified or even recompiled.

### Built-in Access Control

* joynr supports interface and method-level access control using distributed Access Control Lists.

* joynr defines programming interfaces to existing Public Key Infrastructure, in order to reuse infrastructure
already in place for other personalisation and security use cases.

### Language Bindings for C++, Java, JavaScript and TypeScript

* joynr supports the C++, Java and JavaScript languages in order to enable
  developers to target a wide range of platforms, all the way from backend
  servlet-based or even full-stack JEE systems, down to ECUs running in
  vehicles or other embedded platforms.

### Remote Procedure Call (RPC), Publish/Subscribe (pub/sub), Filtered Broadcasts

joynr supports all three of the most common calling paradigms supported by modern transport protocols.

* Remote Procedure Call (RPC) allows modelling services similar to a normal function call
* pub/sub allows data-centeric modelling, when an attribute's value is of interest (Get/Set and Subscriptions)
* Filtered Broadcast or event subscriptions, while similar to pub/sub, allow a richer modelling of events that do not map to a single attribute value

## Layered architecture
joynr employs the following layers in its implementations, decreasing in level of abstraction:

**Formally specified interface**

 * Formal specification of the communication interface of a distributed application, which are defined using the Franca Interface Definition Language (IDL).

**APIs**

Programming interfaces used for discovery, provider registration, or to consume data / services.
 * joynr generators for consumers and providers (C++, Java)
 * Provider registration / discovery API

**Middleware abstraction for RPC / pub/sub / broadcasts**

Full featured communication paradigms mapped to a middleware-agnostic messaging layer (RPC Manager, Pub/Sub Manager, Discovery/Registration, AccessControl)

**Messaging adaption**

Mapping of middleware-agnostic messaging to a specific transport middleware implementation (WebSocket messaging adaption)

**Messaging implementation**

 Open Source industry standard implementations are preferred.
 Currently:
 * mosquitto/hivemq-mqtt-client for MQTT
 * WebSockets

**OSI Transport Layer**

TCP, Sockets, etc. depending on the needs of the messaging implementation.


Per default, joynr ships with a REST-based messaging middleware that addresses requirements such as reliability, stability, performance, memory footprint, CPU usage, and operating system independence required of distributed applications running on embedded devices over unreliable networks.

## Releases
See the [release notes](ReleaseNotes.md) and our [versioning scheme](JoynrVersioning.md).

## Further Reading

[Using joynr](using_joynr.md)
