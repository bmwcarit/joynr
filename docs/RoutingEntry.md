Routing Entry: Objectives and life cycle
======

## Overview
A Routing Entry collects joynr routing information without making a distinction between providers and proxies. More specifically it is composed of:
 * **participantId** // how the provider / proxy is reachable at joynr level
 * **address** // specific address
 * **isGloballyVisible** // if it is globally visible or not
 * **expiryDateMs** // expiryDate associated with it
 * **isSticky** // was provisioned by the user or by joynr during start-up?

The first two are routing specific information, the others are only relevant for bookkeeping operations.

## Operational constraints
1. Trust information received from Global Capabilities Directory (*GCD*):  
whatever comes from GCD is considered trustworthy and should trigger an update in the routing table
1. Only allow one participantId to be associated with a routing address:  
as soon as a participantId is associated with a Routing Entry then this Routing Entry cannot change, unless GCD says so
1. The participantId is unique

## Possible scenarios
These scenarios are used to derive the rules below.
1. A not globally visible provider/proxy is being added / updated
1. A global provider's participantId is added / updated
1. A remote proxy is added / updated based on incoming request to a provider
1. The user provisions capabilities entries
1. All rules with persistency ON or OFF
1. All rules with fixed participantId

The life cycle of a routing entry in the message router is described based on the CRUD set of actions.

## CREATE Rules
A routing entry should be created if the following holds:
* the expiryDate is not in the past **AND**
  * it does not exist **OR**
  * it has the sticky flag set

## READ Rules
A look up in the message router will return a routing entry. The lookup is done via participantId only.

## UPDATE Rules
Update if:
* the expiryDate is not in the past **AND**
  * the routing information (routing address and participantId) stayed the same. **OR**
  * the entry comes from GCD

## DELETE Rules
A routing entry is being deleted from the message router if:
* the entry expired **OR**
* it is not sticky


# OPEN QUESTIONS
how do the rules change with persistency on?  
What happens to provisioned entries?  
How to remove expired proxies?  
Should be possible to remove sticky entries?
