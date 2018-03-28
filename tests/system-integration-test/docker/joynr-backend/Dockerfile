FROM jetty:9-alpine

ENV BACKEND_HOST joynrbackend
ENV JAVA_OPTS -Djoynr.servlet.hostpath=http://joynrbackend:8080 -Djoynr.messaging.channelurldirectoryurl=http://joynrbackend:8080/discovery/channels/discoverydirectory_channelid/ -Djoynr.messaging.bounceproxyurl=http://joynrbackend:8080/bounceproxy -Djoynr.messaging.capabilitiesdirectoryurl=http://joynrbackend:8080/discovery/channels/discoverydirectory_channelid/ -Djavax.net.ssl.trustStore=/ssl/trust.jks -Djavax.net.ssl.keyStore=/ssl/backend.jks -Djavax.net.ssl.keyStorePassword=backend -Djavax.net.ssl.trustStorePassword=backend -Djavax.net.ssl.debug=all

ADD target/single-bounceproxy.war /var/lib/jetty/webapps/bounceproxy.war
