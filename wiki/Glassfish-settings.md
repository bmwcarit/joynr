The following settings changes were made on glassfish 3.1.2.2:

```bash
asadmin create-jvm-options -Dv3.grizzly.cometSupport=true
asdamin set server-config.network-config.protocols.protocol.http-listener-1.http.comet-support-enabled="true"
# allow overriding the default jersey versions shipped with glassfish
asadmin create-jvm-options -Dcom.sun.enterprise.overrideablejavaxpackages=javax.ws.rs,javax.ws.rs.core,javax.ws.rs.ext

# If running joynr applications as servlet (see the discoverydirectoryservlet as an example)
# you need to set the hostPath so that the application can register its channelUrl correctly:
asadmin create-jvm-options -DhostPath="http\://localhost\:8080"
```