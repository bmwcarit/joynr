FROM openjdk:8-jdk-alpine

LABEL com.jfrog.artifactory.retention.maxCount="25"

RUN apk add --no-cache curl

COPY target/graceful-shutdown-test-second-level-provider-*-microbundle.jar /app.jar
COPY src/main/payara/post-boot.txt /post-boot.txt
COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
