FROM openjdk:17.0.1-jdk-slim

COPY target/gcd.jar /gcd.jar
COPY target/log4j*.properties /

ENTRYPOINT ["java", "-Dlog4j2.configurationFile=file:/log4j2.properties", "-jar", "/gcd.jar"]
