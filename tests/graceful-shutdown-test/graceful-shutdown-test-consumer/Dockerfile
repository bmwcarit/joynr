FROM java-11-with-curl:latest

COPY target/graceful-shutdown-test-consumer*.jar /app.jar
COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
