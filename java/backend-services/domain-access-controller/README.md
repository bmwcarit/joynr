# Starting standalone Java Global Domain Access Controller

```
cd $JOYNR_REPOSITORY
mvn clean install
cd java/backend-services/domain-access-controller
java -jar target/deploy/domain-access-controller-jar-with-dependencies.jar
```

This will connect to a MQTT broker on localhost:1883.

The MQTT broker address can also be specified on the command
line by overriding a joynr property as shown below:

```
java -Djoynr.messaging.mqtt.brokerUri="tcp://somehost:1883" \
     -jar target/deploy/domain-access-controller-jar-with-dependencies.jar
```

>Note that the Global Domain Access Controller requires
>a running Global Capabilities Directory in order to globally
>register its providers.

# Shutting down the standalone Java Global Domain Access Controller
The application will shutdown if a network connection is
attempted on localhost:9998 (e.g. via telnet etc.). If required
the port can be modified using the additional command line argument
```
-Djoynr.globaldomainaccesscontrollerlauncher.shutdownport=<port>
```

