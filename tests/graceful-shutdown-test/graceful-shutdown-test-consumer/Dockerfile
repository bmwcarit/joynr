FROM alpine:3.13.6

LABEL com.jfrog.artifactory.retention.maxCount="25"
RUN apk add --no-cache openjdk11
RUN apk add --no-cache curl

COPY target/graceful-shutdown-test-consumer*.jar /app.jar
COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
