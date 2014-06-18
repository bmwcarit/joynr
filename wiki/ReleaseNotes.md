## joynr 0.7.0

### API relevant changes
* Java	The hostPath property can now be set as joynr.servlet.hostPath.
* SSL support for C++ and Java

### Other changes
* C++	libjoynr and cluster-controller now communicate over a single DBus interface
* C++	introduce MessageRouter on libjoynr and cluster-controller side to resolve next hop for messages
* C++	remove EndpointAddress term and use simply Address
* Java	use URL rewriting to implement load balancing on bounce proxy cluster
* Java	enable bounce proxy controller to run in clustered mode
* Java	refactor bounce proxy modules:
	* use Guice injection to configure servlets
	* use RESTful service adapters for messaging related components

## joynr 0.6.0

### API relevant changes
* Java	exceptions: removed checked exceptions from ProxyBuilder
* Java	Check for correct usage of SubscriptionQos
* Java	ChannelUrlDirectoryImpl correctly implements unregisterChannelUrl
* Java	Changes to GlobalCapabilitiesDirectory API causes this version to be incompatible with older server installations.

* C++	joynr is now compatible with Qt 5.2
* C++	read default messaging settings from file
	Default messaging settings are now read from file "resources/default-messaging.settings". 
	When using the find_package command to resolve joynr, it will set the JOYNR_RESOURCES_DIR variable. 
	You can copy the default resources to your bin dir using cmake's file command:
```bash
file(
    COPY ${JOYNR_RESOURCES_DIR}
    DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)
```

### Other changes
* C++	cleaning up joynr libraries used in cmake find_package

* Java	refactored messaging project structure for scalability-related components
* Java	definition of RESTful service adapters for messaging related components
* Java	implementation of lifecycle management and performance monitoring for controlled bounce proxy
* Java	scalability extensions for channel setup at bounce proxy
* Java	scalability extensions for messaging in non-exceptional situations
* Java	backend: improved shutdown responsiveness
* Java	discovery directory servlet: mvn commands to start for local testing
* Java	logging: preparations to allow logging to logstash (distributed logging)
* Java	binary archives (WAR format) of backend components are available on Maven Central
* Java	Enable backend module "MessagingService" to work with joynr messages of unknown content type
* Java	Joynr provides rudimentary embedded database capabilities

* Tooling	Augment features of joynr C++ code generator
* Tooling	Write common util for all generation templates to resolve names of methods, types, interaces, arguments, …