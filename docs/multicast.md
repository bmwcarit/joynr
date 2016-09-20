#Stateless Broadcasts Concept

##Overview

Standard joynr messages are addressed point-to-point, meaning that each joynr
message is addressed to an individual participantId. This addressing paradigm is
well suited to RPC, but in the case of pub/sub and broadcast publications, it
requires the provider process to statefully keep a list of subscribers to which
a publication are to be addressed.

joynr is now introducing multicast joynr messages, defined as a message sent to
a MulticastId. A single message sent to a MulticastId can be received
by 1 to n subscribers to that MulticastId. When used to transmit
publications, MulticastIds allow JEE providers to remain stateless: the
provider no longer has to keep track of listeners to the subscription, but
rather sends directly to the MulticastId. In addition, since only a single
publication message now has to be sent between a provider and its message
broker, MulticastIdes reduce bandwidth use for large sets of subscribers.

The introduction of MulticastIds does not break the joynr APIs for
Consumers or Providers; however, it will extend the API for those applications
wanting to use partitionss. The change will be performed at the Dispatching
(Subscription/PublicationManager) and Messaging (Message Router / Messaging
Skeleton) layers only.

##Technical Details

###MulticastId
* A MulticastId is a composite of the provider's participantId, the name of
  the broadcast as modelled in Franca IDL, and an optional partition.
* A partition is a list of strings that represent a hierarchy similar to a URL
  path or an MQTT (sub)topic.
  A MulticastId can thus be calculated from information available to the
  consumer at discovery.

###Registering to receive multicast messages
When an application subscribes to a broadcast, its libjoynr calls
registerMulticastReceiver with the MulticastId and the receiver's own
libjoynr address.

The cluster controller's Message Router records this information in the
RoutingTable.

The MulticastId is then passed on to the Messaging Stub responsible for the
provider's participantId found in the multicast address.

* For MQTT Skeleton/Stubs, the Messaging Stub passes the call on to its
  Skeleton, which then subscribes to a topic equal to {provider's
  participantId}/{broadcastName}/{partition elements separated "/"}.

* For Channel Skeleton/Stubs, the same concept applies, with the skeleton making
  a new long-poll request to a channelId equal to {provider's
  participantId}{broadcastName}{concatenation of partition elements}.

* For WebSocket Skeleton/Stubs, the WebSocket Messaging Stub sends a special
  message to the provider libjoynr that is used by the provider's libjoynr to
  decide whether to send a publication on the WebSocket. A publication will only
  be sent if a multicast receiver is registered on the cluster controller. A
  subscriber counter in the WebSocket Messaging Stub in the libjoynr keeps track
  of how many subcribers there are on the given MulticastId. TODO define the
  special message.

![Sequence
Diagram](diagrams/SequenceDiagram-Java-MulticastSubscribe.png)

###Sending a multicast message
All OnChange subscriptions and non-selective broadcasts will be sent to a
multicast address.

When the providing application emits a broadcast, the publication is addressed
to the given MulticastId and passed on to the Message Router. No
information about subscribed consumers is required on the provider side.

The Message Router passes the message containing the MulticastId to all
stubs registered in the RoutingTable on the given MulticastId.

* The MQTT Messaging Stub publishes the message to an MQTT topic equal to
  {provider's participantId}/{broadcastName}/{partition elements separated "/"}.

* The ChannelMessagingStub must POST the message to a channel equal to
  {provider's participantId}{broadcastName}{concatenation of partition elements}.

* The WebSocket Messaging Stub transmits the message to its cluster controller
  if the subscriber counter is greater than 0.

###Receiving a multicast message
Incoming messages are passed from the receiving messaging skeleton to the
MessageRouter. The MessageRouter uses the information in the RoutingTable to
multicast the message on all messaging stubs that are registered as receivers on
the given MulticastId. 

![Sequence Diagram](diagrams/SequenceDiagram-Java-MulticastPublish.png)


####Global / Local

Broadcasts can be scoped so that a broadcast emitted by a local provider for
consumption by local consumers only is not transmitted to global participants.
If `local`, then the broadcasted messages are only passed onto skeletons
registered locally in the cluster controller for the relevant MulticastId.

If `global`, then the message is additionally sent out on the primary global
transport registered for the cluster controller. In the case of MQTT this means
publishing the relevant message to the MQTT broker being used by the cluster
controller.

####Prevent infinite echo publication

Because the broadcast messages are sent to a MulticastId rather than
individual participants, the problem for the cluster controller is knowing
whether the message being processed is intended for initial global publication,
or was just received from an outside provider. If it can't make this distinction
it would publish it out to the MQTT broker, just to receive it back again, etc.
etc. etc.

In order to prevent this from happening we plan to include a new transient
attribute in the joynr message which will be set when the message has been
create for initial publication. It then won't be serialised for transmission,
hence it a message arrives without the transient flag sent, it means that it was
received for distribution to the registered listeners only, but not for global
publication.


Issues: ttl of publications can no longer be set by consumer

