This tutorial will guide you through a simple joynr radio application, explaining three essential
joynr concepts:

* A simple radio **communication interface**
* A **consumer** interested in knowing about radio information
* A **provider**, that provides the radio information

# Prerequisites
If you haven't set up the [joynr build environment and infrastructure services]
(Home.md) yet, please do so first.

**ATTENTION** Unfortunately [Franca IDL](https://code.google.com/a/eclipselabs.org/p/franca/)
dependencies are currently not available from [Maven Central Repository](http://search.maven.org/).
Since Franca is needed for joynr code generation, we ship Franca dependencies together with the
joynr source code in the `<JOYNR>/tools/generator/dependency-libs/` directory. Install them into
your local Maven repository by executing the following command:

```bash
~$ cd <JOYNR>/tools/generator/dependency-libs/
<JOYNR>$ mvn install
```

# Exploring the demo
To walk though the radio application, set up the project in your workspace. The example project
contains a java and c++ variation, letting you explore whichever one you find more comfortable.

The RadioApp demo is located in `<JOYNR>/examples/radio-app`. We refer to this location as
`RADIO_HOME`. Before importing the project into Eclipse or QtCreator, let Maven generate the source
files using the command below:

```bash
~$ cd <RADIO_HOME>
<RADIO_HOME>$ mvn generate-sources
```

For exploring the Java code and for viewing the radio communication interface, use Eclipse and
import the RadioApp (`<RADIO_HOME>/pom.xml`)as a Maven project, using the M2E plugin. For C++, open
`<RADIO_HOME>/CMakeLists.txt` in QtCreator.

>**Note: Dependency Resolution**
>
>In Java all dependencies are resolved automatically using Maven.
>In C++ joynr must be [built from the sources](Home.md). Afterwards, joynr will be resolved using
>CMake's `find_package` command:
>
>```cmake
># Pull in the joynr configuration
>find_package(joynr REQUIRED)
>message(STATUS "joynr variable JOYNR_INCLUDE_DIRS=${JOYNR_INCLUDE_DIRS}")
>message(STATUS "joynr variable JOYNR_LIBRARIES=${JOYNR_LIBRARIES}")
>```

With your environment set up, and the RadioApp demo project in your workspace, we are now ready to
explain the three essential ingredients in this demonstration application.

## The Communication Interface (the model)
### What is it?
Communication interfaces are contracts between providers and consumers defining the data and methods
that should be exchanged/communicated via the joynr framework. The communication interfaces are
defined using Franca IDL - from which, supporting joynr library code is generated to enable joynr
communication.

### Radio communication interface
Within this project, we have created a **radio communication interface**. This is located within:
`<RADIO_HOME>/src/main/model`. Within this folder you will see the file: `radio.fidl`, open it.
If a prompt appears asking you whether you want to add the XtextNature, select to add it. This
interface describes the methods and what types of data can be exchanged or subscribed to.

## Generate joynr Code
If you have made changes to the communication interface, the next step is to generate the necessary
joynr library code so that you can start programming your radio interface. This generation process
creates all the necessary code that joynr uses to establish communication.

To generate the source code for the interface, right click on your **project** and select
**Maven->Update Project...**.

>**Note:**
>It is also possible to trigger the code generation process from the command line by running
>`mvn generate-sources`.
>Code generation is triggered by the joynr-generator-maven-plugin. The plugin is configured in the
>plugin section of the <RADIO_HOME>/pom.xml file:
>* the base model is loaded from the classpath through the `io.joynr:basemodel` dependency
>* the custom model is loaded from the `<RADIO_HOME>/src/main/model` directory specified in the
>  model plugin configuration
>* code generation templates for Java and C++ are loaded from the classpath through the
>  `io.joynr.java:java-generator` and `io.joynr.cpp:cpp-generator` dependencies
>* there are two separate plugin executions: one to generate Java and one to generate C++
>
>**Maven joynr Generator Plugin Configuration**
>
>```xml
><plugin>
>	<groupId>io.joynr.tools.generator</groupId>
>	<artifactId>joynr-generator-maven-plugin</artifactId>
>	<executions>
>		<execution>
>			<id>generate-java</id>
>			<phase>generate-sources</phase>
>			<goals>
>				<goal>generate</goal>
>			</goals>
>			<configuration>
>				<model>${basedir}/src/main/model</model>
>				<generationLanguage>java</generationLanguage>
>				<outputPath>${basedir}/src/main/generated-java</outputPath>
>			</configuration>
>		</execution>
>		<execution>
>			<id>generate-cpp</id>
>			<phase>generate-sources</phase>
>			<goals>
>				<goal>generate</goal>
>			</goals>
>			<configuration>
>				<model>${basedir}/src/main/model</model>
>				<generationLanguage>cpp</generationLanguage>
>				<outputPath>${basedir}/src/main/generated-cpp</outputPath>
>			</configuration>
>		</execution>
>	</executions>
>	<dependencies>
>		<dependency>
>			<groupId>io.joynr.java</groupId>
>			<artifactId>java-generator</artifactId>
>			<version>${project.version}</version>
>		</dependency>
>		<dependency>
>			<groupId>io.joynr.cpp</groupId>
>			<artifactId>cpp-generator</artifactId>
>			<version>${project.version}</version>
>		</dependency>
>		<dependency>
>			<groupId>io.joynr</groupId>
>			<artifactId>basemodel</artifactId>
>			<version>${project.version}</version>
>		</dependency>
>	</dependencies>
></plugin>
>```

Now refresh the project folder by hitting F5 and navigate to the
`<RADIO_HOME>/src/main/generated-java` and `<RADIO_HOME>/src/main/generated-cpp` folders.

*All these files within the `<RADIO_HOME>/src/main/generated-java` and
`<RADIO_HOME>/src/main/generated-cpp` folder are the joynr interfaces and classes that you will
require for programming your radio functionality. If you want to communicate between your smart
phone and vehicle, then these files need to be present on both the smart phone, and on the vehicle
side.*

## Providers
A provider is a class that is responsible for being the data source, and supplies data to any
consumers who are interested. Have a look into the provider classes and see how they implement the
`addFavouriteStation` method defined in the `<RADIO_HOME>/src/main/model/radio.fidl` file. They
inherit from the generated classes `joynr.vehicle.RadioAbsctractProvider` (located in
`<RADIO_HOME>/src/main/generated-java/joynr/vehicle/RadioAbstractProvider.java`) and
`joyn::vehicle::RadioProvider` (located in
`<RADIO_HOME>/src/main/generated-cpp/include/joynr/vehicle/RadioProvider.h` and
`<RADIO_HOME>/src/main/generated-cpp/provider/generated/vehicle/RadioProvider.cpp`).

**Java: [\<RADIO_HOME\>/src/main/java/io/joynr/demo/MyRadioProvider.java]
(/examples/radio-app/src/main/java/io/joynr/demo/MyRadioProvider.java)**

```java
...
public class MyRadioProvider extends RadioAbstractProvider {
    ...
    @Override
    public Promise<AddFavouriteStationDeferred> addFavouriteStation(RadioStation radioStation) {
        AddFavouriteStationDeferred deferred = new AddFavouriteStationDeferred();
        LOG.info(PRINT_BORDER + "addFavouriteStation(" + radioStation + ")" + PRINT_BORDER);
        stationsList.add(radioStation);
        deferred.resolve(true);
        return new Promise<AddFavouriteStationDeferred>(deferred);
    }
    ...
}
```

**C++: [\<RADIO_HOME\>/src/main/cpp/MyRadioProvider.h]
(/examples/radio-app/src/main/cpp/MyRadioProvider.h)**

```c++
...
class MyRadioProvider : public joyn::vehicle::RadioProvider {
public:
    ...
    void addFavouriteStation(joynr::RequestStatus& status,
                             bool& returnValue,
                             joynr::vehicle::RadioStation radioStation);
    ...
}
```

**C++: [\<RADIO_HOME\>/src/main/cpp/MyRadioProvider.cpp]
(/examples/radio-app/src/main/cpp/MyRadioProvider.cpp)**

```c++
...
void MyRadioProvider::addFavouriteStation(RequestStatus& status,
                                          bool& returnValue,
                                          vehicle::RadioStation radioStation)
{
    QMutexLocker locker(&mutex);

    MyRadioHelper::prettyLog(
            logger, QString("addFavouriteStation(%1)").arg(radioStation.toString()));
    stationsList.append(radioStation);
    returnValue = true;
    status.setCode(RequestStatusCode::OK);
}
...
```

Have a look at the implementation of these providers, you could imagine writing code here which
links directly to a vehicle/device or to the actual radio station a user is listening to on their
phone.

## Registering Providers
Provider objects are created and registered at joynr in `MyRadioProviderApplication`. The main
method expects the domain to register the provider with as command line argument. The application
can be stopped by hitting `q`. The provider will be unregistered and the application stops.

**C++: [\<RADIO_HOME\>/src/main/cpp/MyRadioProviderApplication.cpp]
(/examples/radio-app/src/main/cpp/MyRadioProviderApplication.cpp)**

```c++
...
JoynRuntime* runtime = JoynRuntime::createRuntime(pathToLibJoynSettings, pathToMessagingSettings);
// Initialise the quality of service settings
// Set the priority so that the consumer application always uses the most recently started provider
types::ProviderQos providerQos;
providerQos.setPriority(QDateTime::currentDateTime().toMSecsSinceEpoch());
// Register the provider
QSharedPointer<MyRadioProvider> provider(new MyRadioProvider(providerQos));
QString authenticationToken("MyRadioProvider_authToken");
runtime->registerCapability<vehicle::RadioProvider>(
        providerDomain, provider, authenticationToken);
...

// Unregister the provider
runtime->unregisterCapability<vehicle::RadioProvider>(
        providerDomain, provider, authenticationToken);

```

**Java: [\<RADIO_HOME\>/src/main/java/io/joynr/demo/MyRadioProviderApplication.java]
(/examples/radio-app/src/main/java/io/joynr/demo/MyRadioProviderApplication.java)**

```java
...
public static void main(String[] args) {
...
    JoynrApplication joynrApplication = new JoynrInjectorFactory(joynrConfig, modules).createApplication(new JoynrApplicationModule(MyRadioProviderApplication.class,
                                                                                                                                    appConfig));
    joynrApplication.run();

    joynrApplication.shutdown();
}

@Override
public void run() {
    provider = new MyRadioProvider();
    runtime.registerCapability(localDomain, provider, AUTH_TOKEN);
}

@Override
public void shutdown() {
    if (provider != null) {
        runtime.unregisterCapability(localDomain, provider, AUTH_TOKEN);
    }
    runtime.shutdown(true);
}
...
```

The following keyboard commands can be used to control the provider application:

* `q` to quit
* `s` to shuffle stations
* `w` to fire weak signal event
* `n` to fire new station discovered event

## Consumers
A consumer is a class that wants to retrieve data from providers, they can either subscribe to data
attribute changes or query providers by calling their operations. In the Radio Demo App you will
find `MyRadioConsumerApplication` class. Open it.

In this demo, we will demonstrate how a consumer can make a call to a provider. Imagine the consumer
lives on your smart phone, and the provider living on your vehicle. Have a look at the run method.
Step through the code to get a feel for the necessary steps in instantiating a proxy that is used to
communicate to your provider. Following from the instantiation of the radio proxy, various calls are
made on the proxy to retrieve different information. Feel free to add a call to the new operation
you added to the interface, and add a print statement so that you can see the result.

**C++: [\<RADIO_HOME\>/src/main/cpp/MyRadioConsumerApplication.cpp]
(/examples/radio-app/src/main/cpp/MyRadioConsumerApplication.cpp)**

```c++
...
int main(int argc, char* argv[])
{
    ...
    JoynrRuntime* runtime =
            JoynrRuntime::createRuntime(pathToLibJoynrSettings, pathToMessagingSettings);

    // Create proxy builder
    ProxyBuilder<vehicle::RadioProxy>* proxyBuilder =
            runtime->getProxyBuilder<vehicle::RadioProxy>(providerDomain);

    // Messaging Quality of service
    qlonglong qosMsgTtl = 30000;                // Time to live is 30 secs in one direction
    qlonglong qosCacheDataFreshnessMs = 400000; // Only consider data cached for < 400 secs

    // Find the provider with the highest priority set in ProviderQos
    DiscoveryQos discoveryQos;
    // As soon as the discovery QoS is set on the proxy builder, discovery of suitable providers
    // is triggered. If the discovery process does not find matching providers within the
    // arbitration timeout duration it will be terminated and you will get an arbitration exception.
    discoveryQos.setDiscoveryTimeout(40000);
    // Provider entries in the global capabilities directory are cached locally. Discovery will
    // consider entries in this cache valid if they are younger as the max age of cached
    // providers as defined in the QoS. All valid entries will be processed by the arbitrator when
    // searching
    // for and arbitrating the "best" matching provider.
    // NOTE: Valid cache entries might prevent triggering a lookup in the global capabilities
    //       directory. Therefore, not all providers registered with the global capabilities
    //       directory might be taken into account during arbitration.
    discoveryQos.setCacheMaxAge(std::numeric_limits<qint64>::max());
    // The discovery process outputs a list of matching providers. The arbitration strategy then
    // chooses one or more of them to be used by the proxy.
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);

    // Build a proxy
    vehicle::RadioProxy* proxy = proxyBuilder->setRuntimeQos(MessagingQos(qosMsgTtl))
                                         ->setProxyQos(ProxyQos(qosCacheDataFreshnessMs))
                                         ->setCached(false)
                                         ->setDiscoveryQos(discoveryQos)
                                         ->build();

    RequestStatus status;

    vehicle::RadioStation currentStation;
    proxy->getCurrentStation(status, currentStation);
    assert(status.successful());
    MyRadioHelper::prettyLog(logger, QString("ATTRIBUTE GET: %1").arg(currentStation.toString()));

    // add favorite radio station
    vehicle::RadioStation favouriteStation("99.3 The Fox Rocks", false, vehicle::Country::CANADA);
    bool success;
    proxy->addFavouriteStation(status, success, favouriteStation);
    MyRadioHelper::prettyLog(
            logger,
            QString("METHOD: added favourite station: %1").arg(favouriteStation.toString()));

    // shuffle the stations
    MyRadioHelper::prettyLog(logger, QString("METHOD: calling shuffle stations"));
    proxy->shuffleStations(status);
}
...
```

**Java: [\<RADIO_HOME\>/src/main/java/io/joynr/demo/MyRadioConsumerApplication.java]
(/examples/radio-app/src/main/java/io/joynr/demo/MyRadioConsumerApplication.java)**

```java
    ...
    public static void main(String[] args) throws IOException {
        ...
        JoynrApplication myRadioConsumerApp = new JoynrInjectorFactory(joynrConfig).createApplication(new JoynrApplicationModule(MyRadioConsumerApplication.class,
                                                                                                                                 appConfig));
        myRadioConsumerApp.run();

        myRadioConsumerApp.shutdown();
    }

    public void run() {
        DiscoveryQos discoveryQos = new DiscoveryQos();
        // As soon as the arbitration QoS is set on the proxy builder, discovery of suitable providers
        // is triggered. If the discovery process does not find matching providers within the
        // arbitration timeout duration it will be terminated and you will get an arbitration exception.
        discoveryQos.setDiscoveryTimeout(10000);
        // Provider entries in the global capabilities directory are cached locally. Discovery will
        // consider entries in this cache valid if they are younger as the max age of cached
        // providers as defined in the QoS. All valid entries will be processed by the arbitrator when searching
        // for and arbitrating the "best" matching provider.
        // NOTE: Valid cache entries might prevent triggering a lookup in the global capabilities
        // directory. Therefore, not all providers registered with the global capabilities
        // directory might be taken into account during arbitration.
        discoveryQos.setCacheMaxAge(Long.MAX_VALUE);
        // The discovery process outputs a list of matching providers. The arbitration strategy then
        // chooses one or more of them to be used by the proxy.
        discoveryQos.setArbitrationStrategy(ArbitrationStrategy.HighestPriority);

        // The provider will maintain at least a minimum interval idle time in milliseconds between
        // successive notifications, even if on-change notifications are enabled and the value changes more
        // often. This prevents the consumer from being flooded by updated values. The filtering happens on
        // the provider's side, thus also preventing excessive network traffic.
        int minInterval_ms = 0;
        // The provider will send notifications every maximum interval in milliseconds, even if the value didn't
        // change. It will send notifications more often if on-change notifications are enabled,
        // the value changes more often, and the minimum interval QoS does not prevent it. The maximum interval
        // can thus be seen as a sort of heart beat.
        int maxInterval_ms = 10000;

        // The provider will send notifications until the end date is reached. The consumer will not receive any
        // notifications (neither value notifications nor missed publication notifications) after
        // this date.
        long expiryDate_ms = System.currentTimeMillis() + 60000;
        // If no notification was received within the last alert interval, a missed publication
        // notification will be raised.
        int alertAfterInterval_ms = 20000;
        // Notification messages will be sent with this time-to-live. If a notification message can not be
        // delivered within its TTL, it will be deleted from the system.
        // NOTE: If a notification message is not delivered due to an expired TTL, it might raise a
        // missed publication notification (depending on the value of the alert interval QoS).
        int publicationTtl_ms = 5000;

        OnChangeWithKeepAliveSubscriptionQos subscriptionQos = new OnChangeWithKeepAliveSubscriptionQos(minInterval_ms,
                                                                                                        maxInterval_ms,
                                                                                                        expiryDate_ms,
                                                                                                        alertAfterInterval_ms,
                                                                                                        publicationTtl_ms);

        ProxyBuilder<RadioProxy> proxyBuilder = runtime.getProxyBuilder(providerDomain, RadioProxy.class);
        try {
            // getting an attribute
            radioProxy = proxyBuilder.setMessagingQos(new MessagingQos()).setDiscoveryQos(discoveryQos).build();
            RadioStation currentStation = radioProxy.getCurrentStation();
            LOG.info(PRINT_BORDER + "ATTRIBUTE GET: current station: " + currentStation + PRINT_BORDER);

            // subscribe to an attribute
            subscriptionIdCurrentStation = radioProxy.subscribeToCurrentStation(new AttributeSubscriptionListener<RadioStation>() {

                                                                                    @Override
                                                                                    public void receive(RadioStation value) {
                                                                                        LOG.info(PRINT_BORDER
                                                                                                + "ATTRIBUTE SUBSCRIPTION: current station: "
                                                                                                + value + PRINT_BORDER);
                                                                                    }

                                                                                    @Override
                                                                                    public void publicationMissed() {
                                                                                        LOG.info(PRINT_BORDER
                                                                                                + "ATTRIBUTE SUBSCRIPTION: publication missed "
                                                                                                + PRINT_BORDER);
                                                                                    }
                                                                                },
                                                                                subscriptionQos);
            ...
            // add favorite radio station
            RadioStation favouriteStation = new RadioStation("99.3 The Fox Rocks", false, Country.CANADA);
            success = radioProxy.addFavouriteStation(favouriteStation);
            LOG.info(PRINT_BORDER + "METHOD: added favourite station: " + favouriteStation + ": " + success
                    + PRINT_BORDER);

            // shuffle the stations
            radioProxy.shuffleStations();
            currentStation = radioProxy.getCurrentStation();
            LOG.info(PRINT_BORDER + "The current radio station after shuffling is: " + currentStation + PRINT_BORDER);
            ...
        } catch (JoynArbitrationException e) {
        } catch (JoynCommunicationException e) {
        }
    }
    ...
```

The following keyboard commands can be used to control the consumer application:

* `q` to quit
* `s` to shuffle stations

## In Action
The radio app can be run in all combinations of consumer and provider: java-java, cpp-cpp, java-cpp,
and cpp-java.

### Starting the Backend
First we need to start the backend services: [Bounceproxy](Home.md#bounceproxy) and
[Discovery](Home.md#discovery-directories). The following Maven command will start a
[Jetty Server](http://eclipse.org/jetty/) on `localhost:8080` and automatically deploy Bounceproxy
and Discovery services:

```bash
<RADIO_HOME>$ mvn jetty:run-war
```

### Java

After importing `<RADIO_HOME>/pom.xml` into Eclipse using the M2E plugin, Eclipse will automatically
resolve dependencies through Maven and build the project.

Now to run the example, first start the provider, open the **MyRadioProviderApplication** class, right
click and **Run as Java Application**. The application will fail to run because the provider domain
must be set on the command line. Right click again and select **Run Configurations...** Go to the
**Arguments** tab and enter the provider domain. Then press **Apply** and then **Run**.

Alternatively, run the provider from the command line by executing the following Maven command:

```bash
<RADIO_HOME>$ mvn exec:java -Dexec.mainClass="io.joynr.demo.MyRadioProviderApplication" -Dexec.args="<my provider domain>"
```

>**Note:**
>The provider domain is used to register the MyRadioProvider on this domain. Consumers must specify
>this domain when creating a proxy for the radio interface in order to use the previously registered
>provider.

Now run the **MyRadioConsumerApplication** class and right click and select **Run as Java
Application**. Add the same provider domain to the run configuration. This consumer will make a call
to the joynr runtime to find a provider with the domain.  If there are several providers of the same
type registered on the same domain, then the ArbitrationStrategy (see in the run method of
MyRadioConsumerApplication class) is used to work out which provider to take. In the console, you
should be able to see log output.

Alternatively, run the consumer from the command line by executing the following Maven command:

```bash
<RADIO_HOME>$ mvn exec:java -Dexec.mainClass="io.joynr.demo.MyRadioConsumerApplication" -Dexec.args="<my provider domain>"
```

### C++

The build files for the project can be generated by running:

**Generating Build Files for CppDemoApp**

```bash
<RADIO_HOME>$ mkdir cpp-build
<RADIO_HOME>$ cd cpp-build
<RADIO_HOME>/cpp-build$ cmake -DENABLE_CLANG_FORMATTER=OFF ..
```

Depending on your environment, the project can then be build by Qt Creator or on the command line.
Pick a domain that will be used to identify the provider. Now to run the example, compile the
project and go to the bin directory:

**Running the Provider**

```bash
<RADIO_HOME>/cpp-build/bin$ ./radio-app-provider-cc <my provider domain>
```

In another terminal window execute:

**Running the Consumer**

```bash
<RADIO_HOME>/cpp-build/bin$ ./radio-app-consumer-cc <my provider domain>
```

This consumer will make a call to the joynr runtime to find a provider with the domain. If there are
several providers of the same type registered on the same domain, then the ArbitrationStrategy (see
in the main function of MyRadioConsumerApplication.cpp) is used to work out which provider to take.

## Summary
In this tutorial, you have seen a communication interface, generated joynr code from it, adapted a
provider and consumer, and seen the communication between the two in action.  The next step is to
create your own interface entirely and create more providers and consumers yourself.  Use this
project as a template for your further investigations!

# Further Reading
* **[Using selective broadcast to implement a geocast](Broadcast-Tutorial.md):**
In this tutorial RadioApp example is extended by a selective broadcast and filter
logics that implements a [geocast](http://en.wikipedia.org/wiki/Geocast).

