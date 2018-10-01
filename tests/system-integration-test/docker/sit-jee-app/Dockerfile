FROM payaradocker/payaraserver:4.1.1.161.1

COPY sit-jee-app.war /sit-jee-app.war
COPY start-me-up.sh /start-me-up.sh

ENV PATH ${PATH}:/opt/payara41/glassfish/bin
RUN echo 'AS_ADMIN_PASSWORD=glassfish\n\
EOF\n' \
>> /tmp/pwdfile
RUN asadmin start-domain && \
	asadmin --user admin --passwordfile=/tmp/pwdfile create-managed-scheduled-executor-service --corepoolsize=100 concurrent/joynrMessagingScheduledExecutor && \
	asadmin --user admin --passwordfile=/tmp/pwdfile deploy /sit-jee-app.war && \
	asadmin --user admin --passwordfile=/tmp/pwdfile set-log-levels io.joynr.systemintegrationtest.jee=FINE && \
	asadmin --user admin --passwordfile=/tmp/pwdfile set-log-levels io.joynr.messaging=FINEST && \
	asadmin --user admin --passwordfile=/tmp/pwdfile set-log-levels io.joynr.dispatching=FINEST && \
	asadmin --user admin --passwordfile=/tmp/pwdfile set-log-levels io.joynr.jeeintegration=FINEST || \
	true

ENTRYPOINT ["/start-me-up.sh"]
