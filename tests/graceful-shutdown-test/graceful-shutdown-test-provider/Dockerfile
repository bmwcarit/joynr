FROM java-11-with-curl:latest

COPY target/graceful-shutdown-test-provider-*-microbundle.jar /app.jar
COPY src/main/payara/post-boot.txt /post-boot.txt
COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
