FROM payaradocker/payaraserver:4.1.1.161.1

COPY target/backpressure-provider-small.war /backpressure-provider-small.war
COPY start-me-up.sh /start-me-up.sh

ENV joynr_servlet_hostpath http://localhost:8080
ENV MQTT_BROKER_URL tcp://mqttbroker:1883

ENV PATH ${PATH}:/opt/payara41/glassfish/bin
RUN echo 'AS_ADMIN_PASSWORD=glassfish\n\EOF\n' \ >> /tmp/pwdfile
RUN asadmin start-domain && \
	asadmin --user admin --passwordfile=/tmp/pwdfile create-managed-scheduled-executor-service --corepoolsize=100 concurrent/joynrMessagingScheduledExecutor && \
	asadmin --user admin --passwordfile=/tmp/pwdfile set-log-attributes com.sun.enterprise.server.logging.GFFileHandler.rotationLimitInBytes=20000000 && \
	asadmin --user admin --passwordfile=/tmp/pwdfile set-log-levels io.joynr.internal=INFO && \
	asadmin --user admin --passwordfile=/tmp/pwdfile set-log-levels io.joynr.messaging=FINE && \
	asadmin --user admin --passwordfile=/tmp/pwdfile set-log-levels io.joynr.dispatching=FINE && \
	asadmin --user admin --passwordfile=/tmp/pwdfile set-log-levels io.joynr.jeeintegration=FINE || \
	true

ENTRYPOINT ["/start-me-up.sh"]
