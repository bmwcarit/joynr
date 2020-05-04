Steps to build/run:

Optional: To avoid having to build everything from the ground up i decided to use the previous version of the Joynr lib v1.13.0 --> For this you have to open the pom.xml in the same folder as this README and change the joynr.version property

1. mvn clean package
2. mvn exec:java -pl java -Dexec.mainClass="io.joynr.helloworld.HelloWorldJava"
