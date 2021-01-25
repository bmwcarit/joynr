FROM openjdk:8-jdk-alpine

LABEL com.jfrog.artifactory.retention.maxCount="25"

COPY target/graceful-shutdown-test-backend-services-*-microbundle.jar /app.jar
COPY src/main/payara/post-boot.txt /post-boot.txt

ENTRYPOINT ["java", "-jar", "/app.jar", "--postbootcommandfile", "/post-boot.txt"]
