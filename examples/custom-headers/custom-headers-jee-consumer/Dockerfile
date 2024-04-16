FROM java-11-with-curl:latest

COPY target/custom-headers-jee-consumer-*-microbundle.jar /app.jar
COPY src/main/payara/post-boot.txt /post-boot.txt

ENTRYPOINT ["java", "-jar", "/app.jar", "--postbootcommandfile", "/post-boot.txt"]
