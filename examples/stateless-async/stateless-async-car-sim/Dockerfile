FROM openjdk:8-jdk-alpine

LABEL com.jfrog.artifactory.retention.maxCount="25"

COPY target/stateless-async-car-sim-*.jar /app.jar

ENTRYPOINT ["java", "-cp", "/app.jar", "io.joynr.examples.statelessasync.carsim.Bootstrap"]
