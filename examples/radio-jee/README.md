# Setup

Deploy the following projects as wars to a local payara instance, being sure to follow the
[deployment instructions](../../java/backend-services/discovery-directory-jee/README.md):

* discovery-directory-jee
* radio-jee-provider
* radio-jee-consumer

# Testing
If using http, use the following commands in this order to test the Radio-Jee app:

    curl -X POST --header "Content-Type:text/plain" --data 'StationOne' http://localhost:8080/radio-jee-consumer/radio-stations/
    curl -X POST http://localhost:8080/radio-jee-consumer/radio-stations/shuffle/
    curl http://localhost:8080/radio-jee-consumer/radio-stations/current-station/

If using https, use the following commands in this order to test the Radio-Jee app:
    curl -X POST -k --header "Content-Type:text/plain" --data 'StationOne' https://localhost:8181/radio-jee-consumer/radio-stations/
    curl -X POST -k https://localhost:8181/radio-jee-consumer/radio-stations/shuffle/
    curl -k https://localhost:8181/radio-jee-consumer/radio-stations/current-station
