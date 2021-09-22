# joynr infrastructure

In order for joynr participants to be able to communicate with
each other, a few infrastructure components are necessary for
them to be able to discover each other and correctly route
messages between the participants.

This documents describes some examples of setting up these
infrastructure components.

## Components

The following component is necessary in order to set up a
joynr environment:

* Global Discovery service (GlobalCapabilitiesDirectory)
   * A joynr based application with which the participants
     register their providers and query to discover other
     participants

Additionally, you have to ensure that any required transport
layer components, such as MQTT brokers, are set up.

## Java

This section describes configuring and starting the JAVA version of the Discovery service.

GlobalCapabilitiesDirectory: see [GCD Readme](../java/backend-services/capabilities-directory/README.md).

## Joynr backend

Please refer to the
[starting joynr backend instructions](../wiki/StartingJoynrBackend.md)

### Logging

If you need to see more detail about communication, or if you are
experiencing problems starting up the applications and want more details
about what is going wrong, then you can set the log levels as follows:

    cd ${GF_HOME}
    bin/asadmin set-log-levels io.joynr=FINE

Of course, you can be more specific about which joynr packages to log
at which level. Also, you can switch logging to `FINEST` for even more
log details.
