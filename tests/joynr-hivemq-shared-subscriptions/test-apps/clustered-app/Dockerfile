FROM openjdk:8-jdk-alpine

RUN apk add --no-cache curl

COPY src/main/payara/post-boot.txt /post-boot.txt
COPY start-me-up.sh /start-me-up.sh
RUN chmod +x /start-me-up.sh

COPY target/sharedsubscriptions-test-clustered-app-*-microbundle.jar /app.jar

ENTRYPOINT ["sh","/start-me-up.sh"]
