This tutorial will guide you through a simple joynr radio application, explaining three essential
joynr concepts:

* A simple radio **communication interface**
* A **consumer** interested in knowing about radio information
* A **provider**, that provides the radio information

# Prerequisites
If you haven't built joynr yet, please do so first:

* [Building joynr Java](java_building_joynr.md) for the Java example only.
* [Building joynr C++](cpp_building_joynr.md) for the C++ example.

This will install the necessary dependencies to your local Maven repository and generate the radio
application source files.

# Exploring the demo
The example project contains a java and c++ variation, letting you explore whichever one you find
more comfortable.

The RadioApp demo is located in `<JOYNR>/examples/radio-app`. We refer to this location as
`RADIO_HOME`.

For exploring the Java code and for viewing the radio communication interface, you can use Eclipse
and import the RadioApp (`<RADIO_HOME>/pom.xml`) as a Maven project, using the M2E plugin. For C++,
open `<RADIO_HOME>/CMakeLists.txt` in QtCreator.

>**Note: Dependency Resolution**
>
>In Java all dependencies are resolved automatically using Maven.
>In C++ joynr must be [built from the sources](cpp_building_joynr.md). Afterwards, joynr will be
>resolved using
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
>* the custom model is loaded from the `<RADIO_HOME>/src/main/model` directory specified in the
>  model plugin configuration
>* code generation templates for Java and C++ are loaded from the classpath through the
>  `io.joynr.java:java-generator` and `io.joynr.cpp:cpp-generator` dependencies
>* there are two separate plugin executions: one to generate Java and one to generate C++

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
`addFavoriteStation` method defined in the `<RADIO_HOME>/src/main/model/radio.fidl` file. They
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
    public Promise<AddFavoriteStationDeferred> addFavoriteStation(RadioStation radioStation) {
        AddFavoriteStationDeferred deferred = new AddFavoriteStationDeferred();
        LOG.info(PRINT_BORDER + "addFavoriteStation(" + radioStation + ")" + PRINT_BORDER);
        stationsList.add(radioStation);
        deferred.resolve(true);
        return new Promise<AddFavoriteStationDeferred>(deferred);
    }
    ...
}
```

**C++: [\<RADIO_HOME\>/src/main/cpp/MyRadioProvider.h]
(/examples/radio-app/src/main/cpp/MyRadioProvider.h)**

```c++
...
class MyRadioProvider : public joyn::vehicle::DefaultRadioProvider {
public:
    ...
    void addFavoriteStation(
        const joynr::vehicle::RadioStation& newFavoriteStation,
        std::function<void(const bool&)> onSuccess,
        std::function<void(const joynr::vehicle::Radio::AddFavoriteStationErrorEnum::Enum&)> onError);
    ...
}
```

**C++: [\<RADIO_HOME\>/src/main/cpp/MyRadioProvider.cpp]
(/examples/radio-app/src/main/cpp/MyRadioProvider.cpp)**

```c++
...
void MyRadioProvider::addFavoriteStation(
    const vehicle::RadioStation& radioStation,
    std::function<void(const bool& returnValue)> onSuccess,
    std::function<void(const joynr::vehicle::Radio::AddFavoriteStationErrorEnum::Enum&)> onError
)
{
    std::lock_guard<std::mutex> locker(mutex);

    MyRadioHelper::prettyLog(
            logger, "addFavoriteStation(" + radioStation.toString() + ")");
    stationsList.push_back(radioStation);
    onSuccess(true);
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
// onFatalRuntimeError callback is optional, but it is highly recommended to provide an
// implementation.
bool isRuntimeOkay = true;
std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onFatalRuntimeError =
        [&] (const joynr::exceptions::JoynrRuntimeException& exception) {
    isRuntimeOkay = false;
    MyRadioHelper::prettyLog(logger, "Unexpected joynr runtime error occurred: " + exception.getMessage());
};

std::shared_ptr<JoynrRuntime> runtime = JoynrRuntime::createRuntime(pathToLibJoynSettings,
  onFatalRuntimeError, pathToMessagingSettings);
// Initialize the quality of service settings
// Set the priority so that the consumer application always uses the most recently started provider
std::chrono::milliseconds millisSinceEpoch =
    std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch());
types::ProviderQos providerQos;
providerQos.setPriority(millisSinceEpoch.count());
// Register the provider
std::shared_ptr<MyRadioProvider> provider(new MyRadioProvider());
runtime->registerProvider<vehicle::RadioProvider>(providerDomain, provider, providerQos);
...

if (isRuntimeOkay) {
    // Unregister the provider
    runtime->unregisterProvider<vehicle::RadioProvider>(providerDomain, provider);
}

return isRuntimeOkay ? 0 : 1;
```

**Java: [\<RADIO_HOME\>/src/main/java/io/joynr/demo/MyRadioProviderApplication.java]
(/examples/radio-app/src/main/java/io/joynr/demo/MyRadioProviderApplication.java)**

```java
...
public static void main(String[] args) {
...
    JoynrApplication joynrApplication = new JoynrInjectorFactory(joynrConfig, runtimeModule,
        new StaticDomainAccessControlProvisioningModule()).createApplication(new JoynrApplicationModule(MyRadioProviderApplication.class, appConfig) {
        ...
    });
    joynrApplication.run();

    joynrApplication.shutdown();
}

@Override
public void run() {
    provider = new MyRadioProvider(providerScope);
    ProviderQos providerQos = new ProviderQos();
    providerQos.setPriority(System.currentTimeMillis());
    runtime.registerProvider(localDomain, provider, providerQos);
}

@Override
public void shutdown() {
    if (provider != null) {
        runtime.unregisterProvider(localDomain, provider);
    }
    runtime.shutdown(true);
}
...
```

The following keyboard commands can be used to control the provider application:

* `q` to quit
* `s` to shuffle stations
* `w` to fire weak signal event
* `p` to fire weak signal event with country of current station as partition
* `n` to fire new station discovered event
* `m` to print status metrics (Note: ConnectionStatusMetrics are only available for HivemqMqttClient)

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
    // onFatalRuntimeError callback is optional, but it is highly recommended to provide an
    // implementation.
    bool isRuntimeOkay = true;
    std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onFatalRuntimeError =
            [&] (const joynr::exceptions::JoynrRuntimeException& exception) {
        isRuntimeOkay = false;
        MyRadioHelper::prettyLog(logger, "Unexpected joynr runtime error occurred: " + exception.getMessage());
    };

    std::shared_ptr<JoynrRuntime> runtime =
            JoynrRuntime::createRuntime(pathToMessagingSettings, onFatalRuntimeError);

    // Create proxy builder
    std::unique_ptr<ProxyBuilder<vehicle::RadioProxy>> proxyBuilder =
            runtime->createProxyBuilder<vehicle::RadioProxy>(providerDomain);

    // Messaging Quality of service
    std::int64_t qosMsgTtl = 30000;                // Time to live is 30 secs in one direction
    std::int64_t qosCacheDataFreshnessMs = 400000; // Only consider data cached for < 400 secs

    // Find the provider with the highest priority set in ProviderQos
    DiscoveryQos discoveryQos;
    // As soon as the discovery QoS is set on the proxy builder, discovery of suitable providers
    // is triggered. If the discovery process does not find matching providers within the
    // discovery timeout duration, discovery will be terminated and you will get a discovery exception.
    discoveryQos.setDiscoveryTimeoutMs(40000);
    // Provider entries in the global capabilities directory are cached locally. Discovery will
    // consider entries in this cache valid if they are younger as the max age of cached
    // providers as defined in the QoS. All valid entries will be processed by the arbitrator when
    // searching
    // for and arbitrating the "best" matching provider.
    // NOTE: Valid cache entries might prevent triggering a lookup in the global capabilities
    //       directory. Therefore, not all providers registered with the global capabilities
    //       directory might be taken into account during arbitration.
    discoveryQos.setCacheMaxAgeMs(std::numeric_limits<qint64>::max());
    // The discovery process outputs a list of matching providers. The arbitration strategy then
    // chooses one or more of them to be used by the proxy.
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);

    // Build a proxy
    std::unique_ptr<vehicle::RadioProxy> proxy = proxyBuilder->setMessagingQos(MessagingQos(qosMsgTtl))
                                                             ->setCached(false)
                                                             ->setDiscoveryQos(discoveryQos)
                                                             ->build();

    vehicle::RadioStation currentStation;
    try {
        proxy->getCurrentStation(status, currentStation);
    } catch (exceptions::JoynrException& e) {
        assert(false);
    }

    MyRadioHelper::prettyLog(logger, "ATTRIBUTE GET: " + currentStation.toString())

    // add favorite radio station
    vehicle::RadioStation favoriteStation("99.3 The Fox Rocks", false, vehicle::Country::CANADA);
    bool success;
    try {
        proxy->addFavoriteStation(success, favoriteStation);
        MyRadioHelper::prettyLog(
            logger,
            "METHOD: added favorite station: " + favoriteStation.toString());
    } catch (exceptions::ApplicationException e) {
        ...
    }

    // shuffle the stations
    MyRadioHelper::prettyLog(logger, "METHOD: calling shuffle stations");
    proxy->shuffleStations();
}

return isRuntimeOkay ? 0 : 1;
...
```

**Java: [\<RADIO_HOME\>/src/main/java/io/joynr/demo/MyRadioConsumerApplication.java]
(/examples/radio-app/src/main/java/io/joynr/demo/MyRadioConsumerApplication.java)**

```java
    ...
    public static void main(String[] args) throws IOException {
        ...
        JoynrApplication myRadioConsumerApp = new JoynrInjectorFactory(joynrConfig, runtimeModule).createApplication(new JoynrApplicationModule(MyRadioConsumerApplication.class,
                                                                                                                                 appConfig) {
            ...
        });
        myRadioConsumerApp.run();

        myRadioConsumerApp.shutdown();
    }

    public void run() {
        DiscoveryQos discoveryQos = new DiscoveryQos();
        // As soon as the discovery QoS is set on the proxy builder, discovery of suitable providers
        // is triggered. If the discovery process does not find matching providers within the
        // discovery timeout duration it will be terminated and you will get a discovery exception.
        discoveryQos.setDiscoveryTimeoutMs(10000);
        // Provider entries in the global capabilities directory are cached locally. Discovery will
        // consider entries in this cache valid if they are younger as the max age of cached
        // providers as defined in the QoS. All valid entries will be processed by the arbitrator when searching
        // for and arbitrating the "best" matching provider.
        // NOTE: Valid cache entries might prevent triggering a lookup in the global capabilities
        // directory. Therefore, not all providers registered with the global capabilities
        // directory might be taken into account during discovery.
        discoveryQos.setCacheMaxAgeMs(Long.MAX_VALUE);
        // The discovery process outputs a list of matching providers. The arbitration strategy then
        // chooses one or more of them to be used by the proxy.
        discoveryQos.setArbitrationStrategy(ArbitrationStrategy.HighestPriority);

        // The provider will maintain at least a minimum interval idle time in milliseconds between
        // successive notifications, even if on-change notifications are enabled and the value changes more
        // often. This prevents the consumer from being flooded by updated values. The filtering happens on
        // the provider's side, thus also preventing excessive network traffic.
        int minIntervalMs = 0;
        // The provider will send notifications every maximum interval in milliseconds, even if the value didn't
        // change. It will send notifications more often if on-change notifications are enabled,
        // the value changes more often, and the minimum interval QoS does not prevent it. The maximum interval
        // can thus be seen as a sort of heart beat.
        int maxIntervalMs = 10000;

        // The provider will send notifications until the end date is reached. The consumer will not receive any
        // notifications (neither value notifications nor missed publication notifications) after
        // this date.
        long expiryDateMs = System.currentTimeMillis() + 60000;
        // If no notification was received within the last alert interval, a missed publication
        // notification will be raised.
        int alertAfterIntervalMs = 20000;
        // Notification messages will be sent with this time-to-live. If a notification message can not be
        // delivered within its TTL, it will be deleted from the system.
        // NOTE: If a notification message is not delivered due to an expired TTL, it might raise a
        // missed publication notification (depending on the value of the alert interval QoS).
        int publicationTtlMs = 5000;

        OnChangeWithKeepAliveSubscriptionQos subscriptionQos = new OnChangeWithKeepAliveSubscriptionQos();
        subscriptionQos.setMinIntervalMs(minInterval_ms).setMaxIntervalMs(maxInterval_ms).setExpiryDateMs(validityMs);
        subscriptionQos.setAlertAfterIntervalMs(alertAfterInterval_ms).setPublicationTtlMs(publicationTtl_ms);

        ProxyBuilder<RadioProxy> proxyBuilder = runtime.getProxyBuilder(providerDomain, RadioProxy.class);
        try {
            // getting an attribute
            radioProxy = proxyBuilder.setMessagingQos(new MessagingQos()).setDiscoveryQos(discoveryQos).build();
            RadioStation currentStation = radioProxy.getCurrentStation();
            LOG.info(PRINT_BORDER + "ATTRIBUTE GET: current station: " + currentStation + PRINT_BORDER);

            // subscribe to an attribute
            subscriptionFutureCurrentStation = radioProxy.subscribeToCurrentStation(new AttributeSubscriptionListener<RadioStation>() {

                                                                                    @Override
                                                                                    public void onReceive(RadioStation value) {
                                                                                        LOG.info(PRINT_BORDER
                                                                                                + "ATTRIBUTE SUBSCRIPTION: current station: "
                                                                                                + value + PRINT_BORDER);
                                                                                    }

                                                                                    @Override
                                                                                    public void onError(JoynrRuntimeException error) {
                                                                                        LOG.info(PRINT_BORDER
                                                                                                + "ATTRIBUTE SUBSCRIPTION: " + error
                                                                                                + PRINT_BORDER);
                                                                                    }
                                                                                },
                                                                                subscriptionQos);
            ...
            // add favorite radio station
            RadioStation favoriteStation = new RadioStation("99.3 The Fox Rocks", false, Country.CANADA);
            success = radioProxy.addFavoriteStation(favoriteStation);
            LOG.info(PRINT_BORDER + "METHOD: added favorite station: " + favoriteStation + ": " + success
                    + PRINT_BORDER);

            // shuffle the stations
            radioProxy.shuffleStations();
            currentStation = radioProxy.getCurrentStation();
            LOG.info(PRINT_BORDER + "The current radio station after shuffling is: " + currentStation + PRINT_BORDER);
            ...
        } catch (DiscoveryException e) {
        } catch (JoynCommunicationException e) {
        }
    }
    ...
```

The following keyboard commands can be used to control the consumer application:

* `q` to quit
* `s` to shuffle stations
* `g` to get the current station
* `l` to get the location of the current station
* `m` to print status metrics (Note: ConnectionStatusMetrics are only available for HivemqMqttClient)

## In Action
The radio app can be run in all combinations of consumer and provider: java-java, cpp-cpp, java-cpp,
and cpp-java.

### Prerequisite
You need to have Maven installed. Joynr is tested with Maven 3.3.3,
but more recent versions should also work here.

For both, consumer and provider, the backend (Global Capabilities Directory) has to be started first.
You will also need to install an MQTT broker, e.g. [Mosquitto](http://mosquitto.org).

### Starting the Backend

The following section describes how to run the joynr MQTT backend service on
(Payara 4.1](http://www.payara.fish).
First, install the application server. It has to be configured once before the first start:

***Payara configuration***
```
Start up the Payara server by changing to the Payara install directory and executing

    bin/asadmin start-domain

Configure your JEE application server with a `ManagedScheduledExecutorService`
resource which has the name `'concurrent/joynrMessagingScheduledExecutor'`:

    bin/asadmin create-managed-scheduled-executor-service --corepoolsize=100 concurrent/joynrMessagingScheduledExecutor

Note the `--corepoolsize=100` option. The default will only create one thread,
which can lead to blocking.

You also need a connection pool for the database which shall be used by the backend service
to persist data.
For this example, we'll create a database on the JavaDB (based on Derby) database which is
installed as part of Payara:

    bin/asadmin create-jdbc-connection-pool \
        --datasourceclassname org.apache.derby.jdbc.ClientDataSource \
        --restype javax.sql.XADataSource \
        --property portNumber=1527:password=APP:user=APP:serverName=localhost:databaseName=joynr-discovery-directory:connectionAttributes=\;create\\=true JoynrPool

Next, create a datasource resource pointing to that database connection. Here's an
example of what that would look like when using the connection pool created above:

    bin/asadmin create-jdbc-resource --connectionpoolid JoynrPool joynr/DiscoveryDirectoryDS

Afterwards you can stop the Payara server by executing

    bin/asadmin stop-domain
```

Start the MQTT broker, and make sure it's accepting traffic on `1883`.

Start the database and the Payara server by changing to the Payara install directory and executing:
```bash
bin/asadmin start-database
bin/asadmin start-domain
```

Finally, fire up the joynr backend service:
```bash
bin/asadmin deploy <RADIO_HOME>/target/discovery-jee.war
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
<RADIO_HOME>$ mvn exec:java -Dexec.mainClass="io.joynr.demo.MyRadioProviderApplication" -Dexec.args="-d <my provider domain>"
```

>**Note:**
>The provider domain is used to register the MyRadioProvider on this domain. Consumers must specify
>this domain when creating a proxy for the radio interface in order to use the previously registered
>provider.

Now run the **MyRadioConsumerApplication** class and right click and select **Run as Java
Application**. Add the same provider domain to the run configuration. This consumer will make a call
to the joynr runtime to find a provider with the domain. If there are several providers of the same
type registered on the same domain, then the ArbitrationStrategy (see in the run method of
MyRadioConsumerApplication class) is used to work out which provider to take. In the console, you
should be able to see log output.

Alternatively, run the consumer from the command line by executing the following Maven command:

```bash
<RADIO_HOME>$ mvn exec:java -Dexec.mainClass="io.joynr.demo.MyRadioConsumerApplication" -Dexec.args="-d <my provider domain>"
```

### C++
Pick a domain that will be used to identify the provider and run the example:

**Running the Provider**

```bash
<CPP_BUILD_DIRECTORY>/radio/bin$ ./radio-app-provider-cc <my provider domain>
```

In another terminal window execute:

**Running the Consumer**

```bash
<CPP_BUILD_DIRECTORY>/radio/bin$ ./radio-app-consumer-cc <my provider domain>
```

>To use a standalone cluster-controller start
>```bash
><CPP_BUILD_DIRECTORY>/radio/bin$ ./cluster-controller
>```
>Then you can start provider-ws and consumer-ws which establish a websocket connection to that
>standalone cluster controller (instead of the cc variants which use an embedded cluster controller):
>```bash
><CPP_BUILD_DIRECTORY>/radio/bin$ ./radio-app-provider-ws <my provider domain>
><CPP_BUILD_DIRECTORY>/radio/bin$ ./radio-app-consumer-ws <my provider domain>
>```
>For UNIX domain socket connection to standalone cluster controller you can start
>provider-uds and consumer-uds:
>```bash
><CPP_BUILD_DIRECTORY>/radio/bin$ ./radio-app-provider-uds <my provider domain>
><CPP_BUILD_DIRECTORY>/radio/bin$ ./radio-app-consumer-uds <my provider domain>
>```

The consumer will make a call to the joynr runtime to find a provider with the domain. If there are
several providers of the same type registered on the same domain, then the ArbitrationStrategy (see
in the main function of MyRadioConsumerApplication.cpp) is used to work out which provider to take.

>**Note**: Since the C\+\+ radio application has been built using docker (
[Building joynr C++](cpp_building_joynr.md)), the shared joynr libraries are only found if your
build directory is ```/data/build/``` as in the docker container. After building joynr C++ with
docker, you can either copy your build directory to this location or add the following directories
to your library path:
>* ```<CPP_BUILD_DIRECTORY>/joynr/bin```
>
>In Linux, this can be achieved by  
>`export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:<BUILD_DIRECTORY>/joynr/bin`  
>Afterwards, the C++ radio application can be started as explained.

## Summary
In this tutorial, you have seen a communication interface, generated joynr code from it, adapted a
provider and consumer, and seen the communication between the two in action. The next step is to
create your own interface entirely and create more providers and consumers yourself. Use this
project as a template for your further investigations!

# Further Reading
* **[Using selective broadcast to implement a geocast](Broadcast-Tutorial.md):**
In that tutorial the example from here is extended by a selective broadcast and filter
logics that implements a [geocast](http://en.wikipedia.org/wiki/Geocast).
