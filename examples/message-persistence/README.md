# Message Persistence Example

This example shows the usage of a simple message persistence implementation to persist queued messages
to prevent their loss if the runtime quits abrubtly (e.g. forcibly terminated from outside without
the runtime having a chance to process all remaining queued messages; see the graceful shutdown
feature for how to quit the runtime and give it a chance to process outstanding messages before
shutting down).

For simplicity the radio app example interface is reused here, so that the current example does
not define its own FIDL file nor it has to generate code. The generated code is taken from the
`radio-jee-api` project as a dependency.

The current example consists of five projects.

* `message-persistence-backend-services` -> a convenience project to easily start the necessary joynr
   infrastructure components (e.g. global discovery directory) for the joynr runtimes to communicate
   with each other
* `message-persistence-impl` -> the simple implementation of the message persistence feature used by
   the example joynr projects
* `message-persistence-java` -> a plain-java joynr provider application using the message persister
   implementation
* `message-persistence-jee` -> a JEE based joynr provider application using the message persister
   implementation
* `message-persistence-consumer` -> A plain-java consumer application which produces messages for the
   java and jee provider applications in order to fill their message queues

## Running

1. Start a local MQTT broker such as `mosquitto`. Make sure it's listening on port `1883`.

2. Inside `message-persistence-backend-services` start up the joynr backend services by calling
`mvn payara-micro:start`. Wait shortly until the applications have fully started.

3. You can now either start the plain-java or the JEE example application. They both use the same
domain/interface for the provided service, so only start one of them up in order to be be sure which
one will receive the joynr messages from the consumer application.
Do this from within either `message-persistence-java` using `mvn exec:java` or from within
`message-persistence-jee` using `mvn payara-micro:start`.

4. Now start the consumer application with `mvn exec:java` from within `message-persistence-consumer`
and wait until it is up and running and producing messages. Check in the provider's `persisted-messages`
directory to see the files representing the messages which are still not processed.

5. Optionally, kill the provider application (`ctrl-C`) during the processing of the requests. If
you start the provider application back up, you will see that the messages which were persisted to
disk are added back to the in-memory queue, processed, and then removed from the persisted cache
(i.e. the files are deleted from the directory).

## Example MessagePersister Implementation Notes

The example implementation is used to demonstrate fulfilling the API and showing how this is used by
joynr at runtime. It is not meant to be used in a real application.

The class `io.joynr.examples.messagepersistence.SimpleFileBasedMessagePersister` in the project
`message-persistence-impl` implements a simple, file-based message persistence which will accept all
messages that are requests. The persistence criteria for real applications must be chosen based on
the business logic, i.e. what messages are important and should not get lost in case of crash. The
persisted messages are then written to the `persisted-messages` directory using the message queue ID
and the ID of the joynr message. Each file contains only the data necessary in order to reconstitute
a `DelayableImmutableMessage` at startup.

Once a message has been processed and `remove` has been called for it, the corresponding file is
deleted from the directory.

The implementation is also deliberately fairly verbose with its logging in order for you to better
see what is being executed and in which order.
