Routing Entry: Objectives and life cycle
======

## Overview
A Routing Entry collects joynr routing information without making a distinction
between providers and proxies. More specifically it is composed of:
 * **participantId**
   the unique identifier of a provider or proxy at joynr level
 * **address**
   defines how the provider is reachable from this runtime
 * **isGloballyVisible**
   whether the participantId is globally visible or not ????
 * **expiryDateMs**
   the expiryDate associated with it - once this date has passed, the entry can be garbage
   collected
 * **isSticky**
   defines whether this entry is protected against garbage collection

The first two elements are routing specific information, the others are only relevant for
bookkeeping operations.

## Address types

An address may be of type
 * **WebSocketClientAddress**  
   the participantId is reachable from this runtime using a connection to the specified websocket client
 * **WebSocketAddress**  
   the participantId is reachable from this runtime using a connection to the specified websocket server
 * **MqttAddress**
   the participantId is reachable from here runtime using a connection to the specified MQTT broker
 * **InProcessAddress**
   internally used

## Operational constraints
1. Each participantId is unique
2. Each participantId has only one RoutingEntry at a time
3. Each RoutingEntry contains exactly one Address
4. If no Routing Entry for a participantId exists, it is always allowed to create one
2. If a Routing Entry for a participantId exists, it can be replaced only if
a. the existing entry is a local entry and the new one is also a local entry
b. the existing entry is a remote entry and the new one is a local entry
c. the existing entry is a remote entry and the new one is also a remote entry
3. If a RoutingEntry is replaced, the longest expiryDate from existing and new entry is kept.
4. If a RoutingEntry is replaced, its isSticky is set to true, if is true in either old or new entry.

## Possible scenarios

1. **A local provider registers locally**
   a. On first time registration, no RoutingEntry entry exists and the new one is created.
   b. On provider restart if re-using its former participantId an existing RoutingEntry gets replaced.
2. **A local provider registers globally**
   a. On first time registration, no RoutingEntry entry exists and the new one is created.
   b. On provider restart re-using its former participantId the existing RoutingEntry gets replaced.
   1. A RoutingEntry with global address could already exist in case during local proxy creation
   a still existing older Global Discovery Entry got discovered before the provider could re-register itself.
   2. A local RoutingEntry could already exist in case the provider had just crashed without unregistering itself.
3. **A provider unregisters**
   a. The RoutingEntry gets removed.
4. **A local proxy is built**
   a. No Routing entry exists and the new one is created (a new proxy always gets a new participantId,
   so that a conflict is not possible)
5. **A global provider is discovered**
   a. This takes places when a proxy is getting created which causes a GDS lookup. Entries
   are getting generated for the providers returned by the lookup request.
   1. If no RoutingEntry exists for a given provider participantId, it is created.
   2. If a RoutingEntry **with global address** already exists for a given provider participantId, it is getting replaced.
   3. If a RoutingEntry **with local address** already exists for a given provider participantId, **it is being kept**.
   This can happen in case a proxy is created with DiscoveryScope that can cause a GDS lookup (i.e. GLOBAL_ONLY,
   LOCAL_AND_GLOBAL, LOCAL_THEN_GLOBAL)
6. **A global proxy is added (incoming request)**
   a. On first time request, no RoutingEntry exists and the new one is created. Furthermore the expiryDate is set according to the expiryDate of the request message.
   b. On any further request, the expiryDate of the existing RoutingEntry gets extended, if applicable.
   c. This allows to garbage collect Routing Entries of no longer used global proxies.
7. **A RoutingEntry expires**
   The Routing Table cleanup job periodically checks for expired Routing Entries and removes them.
