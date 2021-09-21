FROM alpine:3.13.6

LABEL com.jfrog.artifactory.retention.maxCount="25"
RUN apk add --no-cache openjdk11

COPY target/stateless-async-consumer-*.jar /app.jar

ENTRYPOINT ["java", "-cp", "/app.jar", "io.joynr.examples.statelessasync.Bootstrap"]
