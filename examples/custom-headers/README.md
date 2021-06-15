# Custom Headers Example

This example project demonstrates setting and reading back custom headers in joynr.

It consists of the following sub-projects:

* `custom-headers-api`
	* Defines the application's service API using a FIDL file
* `custom-headers-jee-provider`
	* The JEE-based provider implementation of the service
* `custom-headers-jee-consumer`
	* The JEE-based consumer for the service. Provides a control REST API, which can be used to
	trigger the service call

## Scenario

The consumer application puts custom headers into its method call using the `MessagingQos`. The
consumer code shows as well the more low-level possibility to add custom headers to joynr messages
providing a custom implementation of the `JoynrMessageProcessor` (method `processOutgoing`).

The provider application injects the `JoynrJeeMessageMetaInfo` and is able to access the custom
headers packed by the consumer together with the method call. In our example the values of the
custom headers are concatenated and this is the String response of the method call.

It is important to notice that the custom headers as key-value pairs are as well copied into the
reply message of the provider and are therefore available for the consumer's
`JoynrMessageProcessor` (method `processIncoming`) when processing the response message.

## Building the project and the docker containers

This joynr project as well as this example is built using Maven. Simply execute `mvn clean install`
from the root to build all projects. Once the build is finished, use the `docker-build.sh` scripts
in the sub-projects `custom-headers-jee-provider` and `custom-headers-jee-consumer` in order to
create the necessary docker images.

The `joynr-backend-jee` image can be built from
`${joynr_project_root}/tests/joynr-hivemq-shared-subscriptions/joynr-backend-jee/`.

## Running

Use the provided `run_example.sh` script. You will see the response of the method call in the
console. For the detailed logs of the provider and the consumer look into the log file generated at
the end of the example run.
