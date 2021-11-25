# Stateless Async Example

This example project demonstrates stateless asynchronous RPC calls using joynr with a clustered
backend application.

It consists of the following sub-projects:

* `stateless-async-api`
	* Defines the application's service API using a FIDL file
* `stateless-async-car-sim`
	* The provider implementation of the service, simulates a car
	* This runs as a single node and is not clustered
* `stateless-async-consumer`
	* A plain-Java implementation of a consumer
	* This is also not clustered - just used to demonstrate the stateless async API in a plain-Java application
	* Could be extended to be clustered also
* `stateless-async-jee-consumer`
	* The clustered, JEE-based consumer for the service
	* Provides a control REST API, which can be used to trigger the various calls
* `stateless-async-jee-car-sim`
    * A JEE-based version of the car simulator
	* This runs as a single node and is not clustered
	* Can be used in place of the plain-java version

## Scenario

We're simulating the case where we have a vehicle which maintains a set of configurations. The service in
the API project provides methods in order to write and retrieve these configurations using unique IDs.

We have a clustered backend application which provides a REST API for communicating with the vehicle using
that service. The backend applications use the stateless async communication pattern, so that the replies
to the service calls can be handled by any node in the cluster, not just the one from which the request
originated.

This way, the cluster can make use of load balancing and is also more resilient to failure, as, for
example, if the node from which the request originated crashes, any of the other nodes are still able to
deal with the reply from the vehicle.  
Additionally, this better facilitates dynamic scaling, because nodes can be spun down even if they haven't
yet received replies to any requests pending. Equally, if the system is under high load, new nodes can be
spun up and can immediately start handling replies from previous requests sent by other nodes.

## Building

This project is built using Maven. Simply execute `mvn install` from the root to build all projects.

Once the Java applications have been built, use the various `docker-build.sh` scripts in the sub-projects
in order to create the necessary docker images.

## Running

The root of the project contains a `docker-compose.yml`, with service definitions for all actors in the
system required to run an end-to-end test.

First, startup the infrastructure components:

	$ docker-compose up -d hivemq joynr-gcd-db postgresql ; docker-compose logs -f

Wait until no more log entries are written, then start up the joynr backend services:

	$ docker-compose up -d joynr-gcd ; docker-compose logs -f joynr-gcd

Again, wait until it's fully started, then fire up the car simulator:

	$ docker-compose up -d carsim ; docker-compose logs -f carsim

After this has started, fire up a number of JEE consumer applications. E.g. for starting two instances:

	$ docker-compose up -d jee-consumer-1 jee-consumer-2 ; docker-compose logs -f jee-consumer-1 jee-consumer-2

Wait until these have fully started up, then you can commence testing.

## End-to-end testing the service

We recommend using the [HTTPie](https://httpie.org) CLI tool, but you can use any other tool
capable of executing REST calls (e.g. curl or Postman). The snippets documented here are for HTTPie.

In order to see what the various applications are doing, you should tail the logs of the applications
involved. In order to prevent being swamped with log entries, it's recommended to only tail the two JEE
applications, but feel free to inspect the logs of any and all of the other applications, too.

	$ docker-compose logs -f --tail 10 jee-consumer-1 jee-consumer-2

First we need to create a vehicle configuration entry:

	$ cd stateless-async-jee-consumer
	$ http PUT localhost:8081/control/vehicleconfig < test-config.json

You can change the test-config.json and repeat this step if you want to create multiple data entries.

Next, lets retrieve the vehicle configuration we just created:

	$ http localhost:8081/control/vehicleconfig/test-123

You should get the data as a result. If you check the logs, you can see which node handled the callback.
Repeat the step above, and you should see that the other node now has handled the callback, yet we still
retrieve the data from the REST call. This is because the nodes synchronise their data using the
PostgreSQL database.

In order to see fetching attributes working, simply call the following endpoint:

	$ http localhost:8081/control/numberOfConfigs

The REST call itself doesn't return the data, instead it is just logged, so check the application logs
to see which of them handled the callback. Again, repeat this step to verify that the callback is handled
by different nodes each time.

## JEE Consumer Application

The real interesting project in this example is the JEE Consumer Application. When a call to the REST API
is made it results in a request to the joynr service, then it creates a marker entry in the
PostgreSQL DB using the joynr message ID given to it by the stateless async API.
You can see this in the
`io.joynr.examples.statelessasync.RestEndpoint#getById` method, which uses the
`io.joynr.examples.statelessasync.DataAccess` EJB in order to persist a `GetResult` entry with the message ID.

It then uses the `io.joynr.examples.statelessasync.WaitForGetResultBean` in order to poll the database until
the entry for the given message ID is fulfilled. It doesn't matter which node performs this operation.
See the `io.joynr.examples.statelessasync.VehicleStateCallbackBean` for the logic which handles the various
service replies.

## Plain Java Consumer Application

In addition to the JEE consumer application we've provided an example plain-Java consumer
application which demonstrates the use of the stateless async API. If you want to start up the
application, then run:

	$ docker-compose up -d java-consumer ; docker-compose logs -f java-consumer

It will automatically fire off a set of requests to the car sim application and you can see in the
logs that the replies have been processed.

The application will automatically exit after all requests have been processed or timed out.
