FROM alpine:3.13.6

LABEL com.jfrog.artifactory.retention.maxCount="25"
RUN sed -i 's/https/http/g' /etc/apk/repositories
RUN apk add --no-cache openjdk11
RUN apk add --no-cache curl

COPY src/main/payara/post-boot.txt /post-boot.txt
COPY start-me-up.sh /start-me-up.sh
RUN chmod +x /start-me-up.sh

COPY target/backpressure-test-clustered-provider-small-*-microbundle.jar /app.jar

ENTRYPOINT ["sh","/start-me-up.sh"]
