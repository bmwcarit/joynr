FROM sdorra/oracle-java-8

ENV BACKEND_HOST joynrbackend
ENV JAVA_OPTS -Djoynr.servlet.hostpath=http://joynrbackend:8080 -Djoynr.messaging.channelurldirectoryurl=http://joynrbackend:8080/discovery/channels/discoverydirectory_channelid/ -Djoynr.messaging.bounceproxyurl=http://joynrbackend:8080/bounceproxy -Djoynr.messaging.capabilitiesdirectoryurl=http://joynrbackend:8080/discovery/channels/discoverydirectory_channelid/ -Djavax.net.ssl.trustStore=/ssl/trust.jks -Djavax.net.ssl.keyStore=/ssl/backend.jks -Djavax.net.ssl.keyStorePassword=backend -Djavax.net.ssl.trustStorePassword=backend -Djavax.net.ssl.debug=all
ENV CATALINA_HOME /usr/local/apache-tomcat-8.0.32
ENV PATH $CATALINA_HOME/bin:$PATH

#replace /dev/urandom and /dev/random with /dev/./urandom
RUN sed -i 's/dev\/urandom/dev\/.\/urandom/g' $JAVA_HOME/jre/lib/security/java.security
RUN sed -i 's/dev\/random/dev\/.\/urandom/g' $JAVA_HOME/jre/lib/security/java.security

ADD http://archive.apache.org/dist/tomcat/tomcat-8/v8.0.32/bin/apache-tomcat-8.0.32.tar.gz /usr/local/tomcat.tgz

RUN cd /usr/local && tar xzf tomcat.tgz && rm tomcat.tgz
ADD conf/ /usr/local/apache-tomcat-8.0.32/conf
ADD ssl/ /ssl
ADD target/single-bounceproxy.war /usr/local/apache-tomcat-8.0.32/webapps/bounceproxy.war
ADD target/domain-access-controller-servlet.war /usr/local/apache-tomcat-8.0.32/webapps/accesscontrol.war
ADD target/discovery-directory-servlet.war /usr/local/apache-tomcat-8.0.32/webapps/discovery.war
EXPOSE 8080
COPY start_me_up /usr/local/apache-tomcat-8.0.32/
WORKDIR /usr/local/apache-tomcat-8.0.32
ENTRYPOINT ["./start_me_up"]
