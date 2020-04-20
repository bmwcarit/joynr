# Setup

Deploy the following projects as wars to a local payara instance, being sure to follow the
[deployment instructions](../../java/backend-services/discovery-directory-jee/README.md):

* discovery-directory-jee
* radio-jee-provider
* radio-jee-consumer

# Testing
If using http, use the following commands in this order to test the Radio-Jee app:

    // add RadioStation "StationOne" (call .addFavoriteStation)
    curl -X POST --header "Content-Type:text/plain" --data 'StationOne' http://localhost:8080/radio-jee-consumer/radio-stations/
    // call shuffleStations
    curl -X POST http://localhost:8080/radio-jee-consumer/radio-stations/shuffle/
    // call getCurrentStation
    curl http://localhost:8080/radio-jee-consumer/radio-stations/current-station/
    // get location (GeoPosition) of current station
    curl http://localhost:8080/radio-jee-consumer/radio-stations/current-station/location/
    // get country of current station
    curl http://localhost:8080/radio-jee-consumer/radio-stations/current-station/country/

    // fire weakSignal broadcast
    // NOTE: radio-jee-consumer example does not subscribe to broadcasts.
    // It can be tested with the Java/C++/TS radio consumers.
    curl -X POST http://localhost:8080/radio-jee-provider/control/fire-weak-signal/
    // print and get status metrics
    curl http://localhost:8080/radio-jee-provider/control/metrics/

If using https, use the following commands in this order to test the Radio-Jee app:
    curl -X POST -k --header "Content-Type:text/plain" --data 'StationOne' https://localhost:8181/radio-jee-consumer/radio-stations/
    curl -X POST -k https://localhost:8181/radio-jee-consumer/radio-stations/shuffle/
    curl -k https://localhost:8181/radio-jee-consumer/radio-stations/current-station
    curl -k https://localhost:8181/radio-jee-consumer/radio-stations/current-station/location/
    curl -k https://localhost:8181/radio-jee-consumer/radio-stations/current-station/country/

    curl -k -X POST https://localhost:8181/radio-jee-provider/control/fire-weak-signal/
    curl -k https://localhost:8181/radio-jee-provider/control/metrics/

