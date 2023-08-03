# Description

Radio Spring Boot is an example that shows how Spring Boot microservice can run Joynr Standard Java implementation.

# Modules

* radio-spring-boot-api - API module containing Franca interface (same as in radio-app and radio-jee examples) and code generated based on it;
* radio-spring-boot-service - Spring Boot microservice that runs Joynr Standard Java implementation (both provider and consumer);

# Compilation

To build the example project following terminal command should be executed:
```bash
mvn clean install
```

# Launching Spring Boot Joynr Service from terminal

``` bash
cd radio-spring-boot-service
mvn spring-boot:run
```

# Launching Spring Boot Service from IDE

Application class (package io.joynr.examples.spring) should be launched.

# Internal structure

REST controllers and their methods:
* ProviderController:
  * Shuffle stations;
  * Get provider metrics (shuffle stations and get current station invocation count);
* ConsumerController:
  * Shuffle stations;
  * Get current station;

Each controller has its own autowired service for performing operations on provider or consumer.

On application startup (when Spring ApplicationReady event is fired) Joynr runtimes are created and provider and consumer are registered.
Provider and Consumer have separate Joynr runtimes. 

# Testing

End point testing is done via curl.

* Provider testing:
  * Invoke shuffle stations:
    Curl command to invoke end point:
    ``` bash
    curl -X POST localhost:8080/joynr-spring-boot-example/provider/shuffle-stations
    ```
    Response is empty - end point has no return type;
  * Get provider metrics:
    Curl command to invoke end point:
    ``` bash
    curl localhost:8080/joynr-spring-boot-example/provider/metrics
    ```
    Example of response (string):
    Provider statistics:
    Get current radio station invocation count: 0 
    Shuffle radio stations invocation count: 1
* Consumer testing:
  * Invoke shuffle stations:
    Curl command to invoke end point:
    ``` bash
    curl -X POST localhost:8080/joynr-spring-boot-example/consumer/shuffle-stations
    ```
    Example of response (json):
    ``` json
    {
      "_typeName":"joynr.vehicle.RadioStation",
      "name":"Bayern 3",
      "trafficService":true,
      "country":"GERMANY"
    }
    ```
  * Get current station:
    Curl command to invoke end point:
    ``` bash
    curl localhost:8080/joynr-spring-boot-example/consumer/current-station
    ```
    Example of response (json):
    ``` json
    {
      "_typeName":"joynr.vehicle.RadioStation",
      "name":"ABC Trible J",
      "trafficService":true,
      "country":"AUSTRALIA"
    }
    ```