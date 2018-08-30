# Stateless Asynchronous RPC Calls Concept: aka stateless async

The `*Sync` and `*Async` interfaces provide a means of performing point-to-point remote procedure
calls. However, the replies can only be processed by the runtime from which the request originated.
This document describes the implementation for stateless asynchronous calls, where the replies can
be processed by any runtime instance of the same application, e.g. a clustered backend application.

## Interfaces / API

The stateless async implementation provides two new interfaces to allow proxies to make stateless
RPC calls, and register instance-independant callback implementations for dealing with replies.  
Unlike the other RPC calls, the replies can then be handled by any member of a cluster of the same
application, hence enabling better load balancing, failover and scalability.

The `StatelessAsync` interface is the one which you call on the proxy in order to send a stateless
request.

When you make a stateless async call, you provide a message ID callback, which is given the unique
ID of the request being sent, so that you can store context information in a distributed manner,
e.g. in a database, which can then be loaded by any node of the cluster for processing the reply.

The `StatelessAsyncCallback` interface is the one you implement and register with the proxy in order
to process replies received as a result of stateless requests.

When implementing the callback, you need to provide a Use Case name, which should be unique within
that application for the proxy / callback being used. This is so that you can use the same proxy
multiple times in the same application for different business use cases and provide different
callback implementations to be used.

## Technical Details

For the most part the implementation re-uses the same concepts and mechanisms as point-to-point RPC
calls. However, when creating the requests and replies there are some differences, especially
regarding the IDs, addresses and routing.

### Participant ID

Normally, participant IDs are created uniquely for instances in a single runtime. For stateless
calls, it is necessary to create a participant ID which is known throughout a cluster of nodes of
the same application. To this end, we create a participant ID which is made up of the joynr
interface name and the use case name specified by the application developer during registration of
the proxy.  
This way, any node in the cluster can create the same participant ID, and knows which instance
should handle incoming replies.

This takes the form: `[CHANNEL ID] + ':>:' + [INTERFACE NAME] + ':~:' + [USE CASE]`

For example: `io.joynr.tests.statelessasync.jee.consumer:>:tests/statelessasync/VehicleState:~:jee-consumer-test`

In order to save space in the message payload which is sent over the wire, we create a type 3 UUID
of that full participant ID, and store a map from UUID to full participant ID in the runtime.

### Method ID

As the `*StatelessAsyncCallback` instances contain handler methods for all possible methods of a
given joynr interface, and the `Reply` object doesn't contain information about which method was
called, we create a new unique correlation ID to link the outgoing request method to the incoming
reply callback method.

This method ID is added as a suffix to the requestReplyId of the Request payload, which is then
copied to the Reply payload of the response. After extracting the method callback correlation ID
from the incoming requestReplyId, the receiving joynr runtime is then able to pick out the correct
callback method to route the data to.

The stateless requestReplyId format is: `[unique ID] + '#' + [method ID]`

For example: `-5511881354239485001#-875443412` (the unique ID part is a random long, as it only
needs to be unique in the context of the given interface / stateless callback, and not globally; the
method ID is a Java String hashCode of the interfaces method signature for the output parameters)

### Routing

When routing replies, the joynr runtime looks up the relevant address from a table using the
participant ID. In order to be able to route the stateless replies using the new participant ID made
up of interface and use case names, we register the local (in-process) address for the proxy's
stateless participant ID additionally to the normal, globally unique participant ID. This is only
done if the proxy is being registered with a stateless callback handler.

Additionally, when processing incoming stateless replies, the runtime does not remove the reply
caller from the directory if it is a stateless async callback. This way, it can be re-used for
multiple replies without having to be re-added to the directory each time.

The determination as to whether a reply is stateless is made by the presence of the unique method ID
in the request-reply ID (see above).

### JEE Integration

In order to use the stateless async mechanism from JEE applications, you create a bean annotated
with the new `CallbackHandler` qualifier which implements the relevant `*StatelessAsyncCallback`
interface.

You then use the new builder pattern provided by the `ServiceLocator` in order to create the
required proxy, giving it the use case name of the callback handler which should be used for
processing replies. You also need to specify the `*StatelessAsync` interface class as the desired
proxy type, rather than the `*Sync` interface.
