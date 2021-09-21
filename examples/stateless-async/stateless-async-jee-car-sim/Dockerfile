FROM alpine:3.13.6

LABEL com.jfrog.artifactory.retention.maxCount="25"
RUN apk add --no-cache openjdk11

COPY target/stateless-async-jee-car-sim-*-microbundle.jar /app.jar
COPY src/main/payara/post-boot.txt /post-boot.txt

ENTRYPOINT ["java", "-jar", "/app.jar", "--postbootcommandfile", "/post-boot.txt"]
