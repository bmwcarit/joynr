This tutorial will introduce the concept of (selective) broadcasts, how provider and consumers
communicate using broadcasts, and how broadcasts could be used to implement a
[geocast](http://en.wikipedia.org/wiki/Geocast).

Code samples are extracted from the Radio App. If you are not familiar with the Radio App, please
have a look at the [Radio App Tutorial](Tutorial.md) first.

# Broadcast
## What is it?
Broadcasts are triggered by providers and delivered to subscribed consumers. Hence, the semantics
slightly differs from broadcasts you know from computer networking. Broadcasts have an event
character, i.e. providers trigger events and subscribed consumers are notified whenever the event
occurs.

While attributes follow a data-centric Pub/Sub communication paradigm, broadcasts have no data value
assigned that you might get or set. Broadcasts also differ from attribute subscriptions in that a
broadcast may define multiple arguments that are passed from the provider when it triggers the
event. These broadcast arguments are then delivered to subscribed consumers during event
notification.

## Modeling
Broadcasts are defined in the Franca interface. All data types defined in the model can be used to
define broadcast arguments. In the following example, the `weakSignal` broadcast uses a typed
`weakSignalStation` argument to notify subscribed consumers about radio stations with bad signal
quality.

**[\<RADIO_HOME\>/src/main/model/radio.fidl](/examples/radio-app/src/main/model/radio.fidl)**

```
...
<**
	@description: Event that is fired when radio stations have
		weak signal strength.
**>
broadcast weakSignal {
	out {
		RadioTypes.RadioStation weakSignalStation
	}
}
...
```

## Selective (filtered) Broadcast
An extension to broadcasts are so called selective or filtered broadcasts. Providers of selective
broadcasts register additional filter logic that controls whether the broadcast is delivered to a
consumer. The filter logic on provider side can be configured by the consumer via filter
parameters. The following example defines a `newStationDiscovered` selective broadcast with two
arguments: the discovered radio station and the geo position describing where the station was
discovered.

**[\<RADIO_HOME\>/src/main/model/radio.fidl](/examples/radio-app/src/main/model/radio.fidl)**

```
...
<**
	@description: Event that is fired when new radio stations
		are discovered. The event includes information about
		the discovered station and the geo position where the
		station was discovered.
		The interface provider offers a geocast and a traffic
		service filter for this selective broadcast.
		The geocast filter filters events based on position of
		interest and radius of interest area filter parameters
		or does no filtering if these parameters are not set.
		The traffic service filter filters events based on the
		has traffic service filter parameter or does no filtering
		if this parameter is not set.
		NOTE: The joynr middleware calls all filters that are
		registered for the broadcast. Whether a filter actually
		does any filtering or not must be defined on application
		layer (e.g. by interpreting the absence of filter
		parameters as filter deactivation).
	@param: positionOfInterest (RadioTypes.GeoPosition) filter
		parameter that is used by the geocast filter and defines
		the position of interest
	@param: radiusOfInterestArea (Integer) filter parameter that
		is used by the geocast filter and defines the area of
		interest around the position of interest
	@param: hasTrafficService (Boolean) filter parameter that is
		used by the traffic service filter
**>
broadcast newStationDiscovered selective {
	out {
		RadioTypes.RadioStation discoveredStation
		RadioTypes.GeoPosition geoPosition
	}
}
...
```

Currently, Franca does not allow the definition of filter parameters; as a result the filter
parameters are currently defined using Franca structured comments. Note that this approach lacks
type checking. Thus, all parameters are treated as strings on both consumer and provider side.

In the example above, there are three filter parameters defined that belong to two different
filters. `positionOfInterest` and `radiusOfInterest` are read by the geocast filter.
`hasTrafficService`is read by the traffic service filter.


# Geocast
## Filter Logic
To implement the filter logic, a filter class that is specific for every defined broadcast must be
extended. The filter method gets the broadcast arguments and filter parameters as input and returns
true if the broadcast should be delivered to this consumer (i.e. the consumer that provided these
concrete filter parameters).

**Java: [\<RADIO_HOME\>/src/main/java/io/joynr/demo/GeocastBroadcastFilter.java](/examples/radio-app/src/main/java/io/joynr/demo/GeocastBroadcastFilter.java)**

```java
...
public class GeocastBroadcastFilter extends RadioNewStationDiscoveredBroadcastFilter {
    ...
    @Override
    public boolean filter(RadioStation discoveredStation,
                          GeoPosition geoPosition,
                          NewStationDiscoveredBroadcastFilterParameters filterParameters) {
        if (filterParameters.getPositionOfInterest() == null || filterParameters.getRadiusOfInterestArea() == null) {
            // filter parameter not set, so we do no filtering
            return true;
        }

        // put filter logic here:
        // calculate distance between two geo positions using the haversine formula
        // (cf. http://en.wikipedia.org/wiki/Haversine_formula)
        ...
        return distance < radiusOfInterestArea;
    }
}
```

**C++: [\<RADIO_HOME\>/src/main/cpp/GeocastBroadcastFilter.h](/examples/radio-app/src/main/cpp/GeocastBroadcastFilter.h)**

```c++
...
class GeocastBroadcastFilter : public vehicle::RadioNewStationDiscoveredBroadcastFilter
{
public:
    GeocastBroadcastFilter();

    virtual bool filter(
            const joynr::vehicle::RadioStation& discoveredStation,
            const joynr::vehicle::GeoPosition& geoPosition,
            const vehicle::RadioNewStationDiscoveredBroadcastFilterParameters& filterParameters);
};
```

**C++: [\<RADIO_HOME\>/src/main/cpp/GeocastBroadcastFilter.cpp](/examples/radio-app/src/main/cpp/GeocastBroadcastFilter.cpp)**

```c++
...
bool GeocastBroadcastFilter::filter(
        const joynr::vehicle::RadioStation& discoveredStation,
        const joynr::vehicle::GeoPosition& geoPosition,
        const vehicle::RadioNewStationDiscoveredBroadcastFilterParameters& filterParameters)
{
    if (filterParameters.getPositionOfInterest().isNull() ||
        filterParameters.getRadiusOfInterestArea().isNull()) {
        // filter parameter not set, so we do no filtering
        return true;
    }

    // put filter logic here:
    // calculate distance between two geo positions using the haversine formula
    // (cf. http://en.wikipedia.org/wiki/Haversine_formula)
    ...
    return distance < radiusOfInterestArea;
}
```

Filters must be registered with the provider object.

**Java: [\<RADIO_HOME\>/src/main/java/io/joynr/demo/MyRadioProviderApplication.java](/examples/radio-app/src/main/java/io/joynr/demo/MyRadioProviderApplication.java)**

```java
...
provider = new MyRadioProvider();
provider.addBroadcastFilter(new TrafficServiceBroadcastFilter());
provider.addBroadcastFilter(new GeocastBroadcastFilter(jsonSerializer));
...
```

**C++: [\<RADIO_HOME\>/src/main/cpp/MyRadioProviderApplication.cpp](/examples/radio-app/src/main/cpp/MyRadioProviderApplication.cpp)**

```c++
...
// create provider instance
std::shared_ptr<MyRadioProvider> provider(new MyRadioProvider());
// add broadcast filters
std::shared_ptr<TrafficServiceBroadcastFilter> trafficServiceBroadcastFilter(
        new TrafficServiceBroadcastFilter());
provider->addBroadcastFilter(trafficServiceBroadcastFilter);
std::shared_ptr<GeocastBroadcastFilter> geocastBroadcastFilter(new GeocastBroadcastFilter());
provider->addBroadcastFilter(geocastBroadcastFilter);
...
```

If multiple filters are registered on the same provider and broadcast, all filters are applied in a
chain and the broadcast is only delivered to the consumer if all filters in the chain return true.

## Subscribing to selective broadcasts
Consumers that subscribe to a selective broadcast might supply filter parameters to control
broadcast delivery. It is up to the filter logic developer to define the behavior if not all filter
parameters are provided. However, it is good practice to do no filtering in this case (i.e.
returning true).

**Java: [\<RADIO_HOME\>/src/main/java/io/joynr/demo/MyRadioConsumerApplication.java](/examples/radio-app/src/main/java/io/joynr/demo/MyRadioConsumerApplication.java)**

```java
...
OnChangeSubscriptionQos newStationDiscoveredBroadcastSubscriptionQos;
int nsdbMinIntervalMs = 2 * 1000;
long nsdbValidityMs = 180 * 1000;
int nsdbPublicationTtlMs = 5 * 1000;
newStationDiscoveredBroadcastSubscriptionQos = new OnChangeSubscriptionQos();
newStationDiscoveredBroadcastSubscriptionQos.setMinIntervalMs(nsdbMinIntervalMs).setValidityMs(nsdbValidityMs).setPublicationTtlMs(nsdbPublicationTtlMs);
NewStationDiscoveredBroadcastFilterParameters newStationDiscoveredBroadcastFilterParams = new NewStationDiscoveredBroadcastFilterParameters();
newStationDiscoveredBroadcastFilterParams.setHasTrafficService("true");
GeoPosition positionOfInterest = new GeoPosition(48.1351250, 11.5819810); // Munich
String positionOfInterestJson = null;
try {
    positionOfInterestJson = objectMapper.writeValueAsString(positionOfInterest);
} catch (JsonProcessingException e1) {
    LOG.error("Unable to write position of interest filter parameter to JSON", e1);
}
newStationDiscoveredBroadcastFilterParams.setPositionOfInterest(positionOfInterestJson);
newStationDiscoveredBroadcastFilterParams.setRadiusOfInterestArea("200000"); // 200 km
radioProxy.subscribeToNewStationDiscoveredBroadcast(new RadioBroadcastInterface.NewStationDiscoveredBroadcastAdapter() {
                                                        @Override
                                                        public void onReceive(RadioStation discoveredStation,
                                                                            GeoPosition geoPosition) {
                                                            LOG.info(PRINT_BORDER
                                                                    + "BROADCAST SUBSCRIPTION: new station discovered: "
                                                                    + discoveredStation + " at "
                                                                    + geoPosition + PRINT_BORDER);
                                                        }
                                                    },
                                                    newStationDiscoveredBroadcastSubscriptionQos,
                                                    newStationDiscoveredBroadcastFilterParams);
...
```

**C++: [\<RADIO_HOME\>/src/main/cpp/MyRadioConsumerApplication.cpp](/examples/radio-app/src/main/cpp/MyRadioConsumerApplication.cpp)**
```c++
...
auto newStationDiscoveredBroadcastSubscriptionQos = std::make_shared<OnChangeSubscriptionQos>();
newStationDiscoveredBroadcastSubscriptionQos->setMinInterval(2 * 1000);
newStationDiscoveredBroadcastSubscriptionQos->setValidity(180 * 1000);
std::shared_ptr<ISubscriptionListener<vehicle::RadioStation, vehicle::GeoPosition>>
        newStationDiscoveredBroadcastListener(new NewStationDiscoveredBroadcastListener());
vehicle::RadioNewStationDiscoveredBroadcastFilterParameters
        newStationDiscoveredBroadcastFilterParams;
newStationDiscoveredBroadcastFilterParams.setHasTrafficService("true");
vehicle::GeoPosition positionOfInterest(48.1351250, 11.5819810); // Munich
std::string positionOfInterestJson(joynr::serializer::serializeToJson(positionOfInterest));
newStationDiscoveredBroadcastFilterParams.setPositionOfInterest(positionOfInterestJson);
newStationDiscoveredBroadcastFilterParams.setRadiusOfInterestArea("200000"); // 200 km
proxy->subscribeToNewStationDiscoveredBroadcast(newStationDiscoveredBroadcastFilterParams,
                                                newStationDiscoveredBroadcastListener,
                                                newStationDiscoveredBroadcastSubscriptionQos);
...
```

## Triggering Broadcasts
Providers of a communication interface trigger broadcasts that are defined in the communication
interface by calling `fire<Broadcast name>` methods defined in `<Interface name>AbstractProvider`.
They must supply the broadcast arguments that are delivered to the consumers.

**Java: [\<RADIO_HOME\>/src/main/java/io/joynr/demo/MyRadioProvider.java](/examples/radio-app/src/main/java/io/joynr/demo/MyRadioProvider.java)**

```java
...
RadioStation discoveredStation = currentStation;
GeoPosition geoPosition = countryGeoPositionMap.get(discoveredStation.getCountry());
fireNewStationDiscovered(discoveredStation, geoPosition);
...
```

**C++: [\<RADIO_HOME\>/src/main/cpp/MyRadioProvider.cpp](/examples/radio-app/src/main/cpp/MyRadioProvider.cpp)**

```c++
...
vehicle::RadioStation discoveredStation(stationsList.at(currentStationIndex));
vehicle::GeoPosition geoPosition(countryGeoPositionMap.at(discoveredStation.getCountry()));
fireNewStationDiscovered(discoveredStation, geoPosition);
...
```
