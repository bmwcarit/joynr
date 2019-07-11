<p align="center">
<img src="graphics/joynr-logo.png" alt="joynr" width="300"/>
</p>

# What is joynr?
joynr is a web-based communication framework for Java, C++, and JavaScript applications
wanting to interact with other applications, no matter whether they're deployed on consumer
devices, vehicles, backend infrastructure or in the cloud.

joynr makes writing distributed applications easy, as it:

* takes care of determining the most appropriate communication paradigm to talk with the desired
	end point
* provides a simple application programming interface to the joynr framework
* speeds up integration of new applications

... allowing you to focus on building your distributed application.

# How do I write joynr?
joynr is correctly written as "joynr". It is not an abbreviation nor a combination of words.

# Documentation
Have a peek at our documentation for more information:
* [Get to know joynr](wiki/Home.md) -- Introduction to joynr
* [Using joynr](wiki/using_joynr.md) -- General information about joynr
	* [Franca IDL overview](wiki/franca.md) -- Introduction to Franca Interface Definition Language
		used for modelling
	* [joynr Code Generator](wiki/generator.md) -- Using the joynr Code Generator to generate code
		from the Franca model files
	* [Infrastructure](wiki/infrastructure.md) -- Setting up the joynr infrastructure components for your environment
	* [Multiple backends](wiki/multiple-backends.md) -- joynr with multiple backends (multiple global connections)
* Building joynr
	* [Building joynr Java](wiki/java_building_joynr.md) -- Building joynr Java and common components
		yourself
	* [Building joynr C++](wiki/cpp_building_joynr.md) -- Building joynr C++ yourself
	* [Building joynr JavaScript](wiki/javascript_building_joynr.md) -- Building joynr JavaScript yourself
* Developer documentation
	* Java and JEE
		* [Java Developer Guide](wiki/java.md) -- Developing Java applications with joynr
		* [Java Configuration Reference](wiki/JavaSettings.md) -- a reference of the available
			configuration options for Java and JEE joynr applications
		* [MQTT Clients](wiki/java_mqtt_clients.md) -- how to choose and configure the MQTT client to
			use for a Java runtime.
		* [JEE Developer Guide](wiki/jee.md) -- Developing JEE applications with joynr
		* [Glassfish Settings](wiki/Glassfish-settings.md) -- Settings you need to run joynr backend
			services on Glassfish application servers
	* Android
		* [Android Developer Guide](wiki/Android.md) -- Developing Android applications with joynr
	* C++
		* [C++ Developer Guide](wiki/cplusplus.md) -- Developing C++ applications with joynr
		* [C++ cluster-controller Settings](wiki/ClusterControllerSettings.md) -- C++ cluster-controller settings
	* JavaScript / TypeScript
		* [JavaScript Developer Guide](wiki/javascript.md) -- Developing JavaScript applications with joynr
		* [JavaScript Configuration Reference](wiki/JavaScriptSettings.md) -- a reference of the available
			configuration options for JavaScript and TypeScript joynr applications
        * *For details about running the joynr Javascript tests see [joynr Javascript Testing](javascript_testing.md)*
* Tutorials
	* [Radio App Tutorial](wiki/Tutorial.md) -- Create your first joynr app: define a communication
		interface, implement a provider, and create a consumer
	* [Broadcast Tutorial](wiki/Broadcast-Tutorial.md) -- Learn about (selective) broadcasts and
		implement a geocast
	* [JavaScript Tutorial](wiki/JavaScriptTutorial.md) -- Create your first joynr JavaScript application
* [Distribution](wiki/Distribution.md) -- How to get joynr
* [Versioning scheme](wiki/JoynrVersioning.md) -- Meaning of joynr version numbers
* [Release Notes](wiki/ReleaseNotes.md) -- All joynr releases and API changes at a glance

