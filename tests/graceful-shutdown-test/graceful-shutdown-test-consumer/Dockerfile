FROM openjdk:8-jdk-alpine

RUN apk add --no-cache curl

COPY target/graceful-shutdown-test-consumer*.jar /app.jar
COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
