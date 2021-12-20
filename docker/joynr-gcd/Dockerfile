FROM openjdk:11-jre-slim

COPY target/gcd.jar /gcd.jar
COPY target/log4j*.properties /

ENTRYPOINT ["java", "-Dlog4j.configuration=file:/log4j2.properties", "-jar", "/gcd.jar"]
