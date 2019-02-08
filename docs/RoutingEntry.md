Routing Entry: Objectives and life cycle
======

## Overview
A Routing Entry collects joynr routing information without making a distinction
between providers and proxies. More specifically it is composed of:
 * **participantId**
   The unique identifier of a provider or proxy at joynr level.
 * **address**
   Defines how the provider is reachable from this runtime.
 * **isGloballyVisible**
   Whether the participantId is globally visible or not.
   *This information is required for the routing of multicast publications and for the handling
   subscriptions to multicsts. The details are out of the scope of this document.*
   * a provider is globally visible when it is registered globally
   * a proxy is globally visible when it is connected to a global provider or it has discovered
     a local provider globally or the arbitrator has selected the global DiscoveryEntry
     (might happen with DiscoveryScope: `GLOBAL`, `LOCAL_AND_GLOBAL` or `LOCAL_THEN_GLOBAL`)
 * **expiryDateMs**
   The expiryDate associated with it - once this date has passed, the entry can be garbage
   collected.
 * **isSticky**
   Defines whether this entry is protected against garbage collection and overwriting. Only
   provisioned entries are sticky.

The first two elements are routing specific information, the others are only relevant for
bookkeeping operations.

## Address types
An address may be of type
 * **WebSocketClientAddress**  
   the participantId is reachable from this runtime using a connection to the specified websocket
   client (libjoynr runtime)
 * **WebSocketAddress**  
   the participantId is reachable from this runtime using a connection to the specified websocket
   server (cluster controller)
 * **MqttAddress**/**ChannelAddress**  
   the participantId is reachable from this runtime (cluster controller) using a connection to the
   specified MQTT broker/Http Bounceproxy
 * **InProcessAddress**  
   the participantId is in the same runtime
 * **BrowserAddress**  
   NOT USED

## Precedence of address types
* **InProcessAddress** has precedence over all other address types:  
  because participants in the same process and runtime are most trustworthy and the routing
  information of inprocess participants, especially joynr internal providers, e.g. routing or
  discovery provider in the cluster controller, must not be overwritten by any other address from
  outside.  
  This also allows restart of consumer and provider applications in the same runtime
  even if the provider is registered after it has been discovered by the consumer. In this case,
  the consumer discovers the old DiscoveryEntry of the provider and adds a routing entry for the
  in process provider with the cluster controller's WebSocketAddress. When the provider registers
  again, the WebSocketAddress is overwritten by the correct InProcessAddress of the provider and
  the routing works as expected.
* **WebSocketClientAddress** has precedence over all address types **except InProcessAddress**:  
  A WebSocketClientAddress addresses local (in terms of a cluster controller) clients. It must not
  be overwritten by a global address which could cause a loop, see the following sections.
* **WebSocketAddress** has precedence over global addresses (MqttAddress, ChannelAddress):  
  It must not overwrite WebSocketClientAddress and InProcessAddress because
  * In the cluster controller, a WebSocketAddress which addresses the cluster controller itself
    makes no sense
  * In a libjoynr runtime, a WebSocketAddress should theoretically have precedence over
    WebSocketClientAddress. However, the cluster controller is more important and a libjoynr runtime
    never adds a WebSocketClientAddress to the routing table.
* **MqttAddress** and **ChannelAddress** (global address types) have no precedence over other
  address types

## Operational constraints
1. Each participantId is unique
2. Each participantId has only one RoutingEntry at a time
3. Each RoutingEntry contains exactly one address
4. If no Routing Entry for a participantId exists, it is always allowed to create one
5. Sticky Routing Entries (provisioned routing entries) cannot be replaced or removed
6. If a Routing Entry for a participantId exists, it can be replaced only if (see also
   [section Precedence of address types](#precedence-of-address-types))
   1. the existing entry and the new entry contain the same address **OR**
   2. the new entry is an InProcess entry **OR**
   3. the existing entry is a WebSocket, WebSocketClient or remote entry (MqttAddress,
      ChannelAddress) and the new one is a WebSocketClient entry **OR**
   4. the existing entry is a remote entry and the new one is WebSocket entry **OR**
   5. the existing entry is a remote entry and the new one is also a remote entry
6. If a RoutingEntry is replaced, the expiryDateMs attribute and the isSticky flag are merged
8. If a Routing Entry for a participant exists and a new entry contains the same address and
   isGloballyVisible flag, only the expiryDateMs attribute and the isSticky flag are merged
7. If the expiryDateMs attribute and isSticky flag of two routing entries are merged,
   * the longest expiryDate from existing and new entry is kept.
   * the isSticky flag is set to true, if it is true in either old or new entry.

## Possible scenarios
1. **An inprocess provider registers locally**
   a. On first time registration, no RoutingEntry entry exists and the new one is created.
   b. On provider restart if re-using its former participantId an existing RoutingEntry gets
      replaced.
      1. A WebSocketClient entry could already exist (in the clustercontroller) if a local provider
         in a (websocket) libjoynr runtime has registered itself with a participantId of a joynr
         internal provider of the clustercontroller, e.g. RoutingProvider, (local)
         DiscoveryProvider.  
         *This should never happen if joynr is used correctly and id not considered in the following
         scenarios.*
      2. A WebSocket RoutingEntry could already exist in a libjoynr runtime if a local proxy for a
         provider in the same process (and runtime) is created before the provider has
         (re-)registered itself after the restart, i.e. the provider is discovered via its old
         DiscoveryEntry from the previous run.
2. **An inprocess provider registers globally**
   a. On first time registration, no RoutingEntry entry exists and the new one is created.
   b. On provider restart re-using its former participantId the existing RoutingEntry gets replaced.
      1. see 1.b.1.
      2. see 1.b.2.
      3. A global RoutingEntry (with global address) could already exist in case during local proxy
         creation a still existing older Global Discovery Entry got discovered before the provider
         could re-register itself.
3. **A websocket (client) provider registers locally**
   a. On first time registration, no RoutingEntry entry exists and the new one is created.
   b. On provider restart if re-using its former participantId an existing RoutingEntry gets replaced
      if it is not an InProcess entry (see above).
      1. A WebSocketClient RoutingEntry could already exist in case the provider had just crashed
         without unregistering itself.
      2. see 1.b.2.
4. **A websocket (client) provider registers globally**
   a. On first time registration, no RoutingEntry entry exists and the new one is created.
   b. On provider restart re-using its former participantId the existing RoutingEntry gets replaced
      if it is not an InProcess entry (see above).
      1. see 3.b.1.
      2. see 1.b.2.
      3. see 2.b.3.
3. **A provider unregisters**
   The RoutingEntry gets removed.
4. **A proxy is built for a local provider (a local provider is discovered)**
   No Routing entry exists for the proxy and the new one is created (a new proxy always gets a new
   participantId, so that a conflict is not possible)
5. **A proxy is build for a global provider (a global provider is discovered)**
   No Routing entry exists for the proxy and the new one is created (a new proxy always gets a new
   participantId, so that a conflict is not possible)  
   The global proxy creation also causes a GCD lookup and Routing Entries are getting generated for
   the providers returned by the lookup request.  
   This can happen also for local providers in case a proxy is created with DiscoveryScope that can
   cause a GCD lookup (i.e. GLOBAL_ONLY, LOCAL_AND_GLOBAL, LOCAL_THEN_GLOBAL)
      1. If no RoutingEntry exists for a given provider participantId, it is created.
      2. If a RoutingEntry **with global address** already exists for a given provider participantId,
         it is getting replaced.
      3. If a RoutingEntry **with local address** already exists for a given provider participantId,
         **it is being kept**.
6. **A global proxy is added (from the replyTo address of an incoming request)**
   a. On first time request, no RoutingEntry exists and the new one is created. Furthermore the
      expiryDate is set according to the expiryDate of the request message.
   b. On any further request, the expiryDate of the existing RoutingEntry gets extended, if applicable.
   c. This allows to garbage collect Routing Entries of no longer used global proxies.
7. **A RoutingEntry expires**
   The Routing Table cleanup job periodically checks for expired Routing Entries and removes them
   (if they are not sticky).
