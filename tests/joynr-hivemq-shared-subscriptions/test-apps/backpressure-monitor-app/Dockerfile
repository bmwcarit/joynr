FROM java-11-with-curl:latest

RUN sed -i 's/https/http/g' /etc/apk/repositories

COPY src/main/payara/post-boot.txt /post-boot.txt
COPY start-me-up.sh /start-me-up.sh
RUN chmod +x /start-me-up.sh

COPY target/backpressure-test-monitor-app-*-microbundle.jar /app.jar

ENTRYPOINT ["sh","/start-me-up.sh"]
