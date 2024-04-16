FROM java-11-with-curl:latest

COPY target/stateless-async-consumer-*.jar /app.jar

ENTRYPOINT ["java", "-cp", "/app.jar", "io.joynr.examples.statelessasync.Bootstrap"]
