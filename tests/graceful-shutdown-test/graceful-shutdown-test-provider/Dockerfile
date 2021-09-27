FROM alpine:3.13.6

LABEL com.jfrog.artifactory.retention.maxCount="25"
RUN apk add --no-cache openjdk11
RUN apk add --no-cache curl

COPY target/graceful-shutdown-test-provider-*-microbundle.jar /app.jar
COPY src/main/payara/post-boot.txt /post-boot.txt
COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
