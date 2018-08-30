FROM openjdk:8-jdk-alpine

COPY target/stateless-async-consumer-*.jar /app.jar

ENTRYPOINT ["java", "-cp", "/app.jar", "io.joynr.examples.statelessasync.Bootstrap"]
