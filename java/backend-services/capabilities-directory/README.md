# Starting a standalone Java Global Capabilities Directory

```
cd $JOYNR_REPOSITORY
mvn clean install
cd java/backend-services/capabilities-directory
java -jar target/deploy/capabilities-directory-jar-with-dependencies.jar
```

This will connect to a MQTT broker on localhost:1883.

The MQTT broker address can also be specified on the command
line by overriding a joynr property as shown below:

```
java -Djoynr.messaging.mqtt.brokerUri="tcp://somehost:1883" \
     -jar target/deploy/capabilities-directory-jar-with-dependencies.jar
```

# Shutting down the standalone Java Global Capabilities Directory

The application will shutdown if a network connection is attempted on localhost:9999 (e.g. via
telnet etc.). If required, the port can be modified using the additional command line argument
```
-Djoynr.capabilitiesdirectorylauncher.shutdownport=<port>
```

