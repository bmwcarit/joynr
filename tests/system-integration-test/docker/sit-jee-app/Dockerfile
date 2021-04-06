FROM payaradocker/payaraserver:4.1.1.161.1

LABEL com.jfrog.artifactory.retention.maxCount="25"

COPY sit-jee-app.war /sit-jee-app.war
COPY start-me-up.sh /start-me-up.sh

ENV PATH ${PATH}:/opt/payara41/glassfish/bin
RUN echo 'AS_ADMIN_PASSWORD=glassfish\n\
EOF\n' \
>> /opt/payara41/pwdfile
RUN asadmin --user admin --passwordfile=/opt/payara41/pwdfile start-domain && \
    asadmin --user admin --passwordfile=/opt/payara41/pwdfile create-managed-scheduled-executor-service --corepoolsize=100 concurrent/joynrMessagingScheduledExecutor && \
    asadmin --user admin --passwordfile=/opt/payara41/pwdfile set-log-levels io.joynr.systemintegrationtest.jee=FINE && \
    asadmin --user admin --passwordfile=/opt/payara41/pwdfile set-log-levels io.joynr.messaging=FINEST && \
    asadmin --user admin --passwordfile=/opt/payara41/pwdfile set-log-levels io.joynr.dispatching=FINEST && \
    asadmin --user admin --passwordfile=/opt/payara41/pwdfile set-log-levels io.joynr.discovery=FINEST && \
    asadmin --user admin --passwordfile=/opt/payara41/pwdfile set-log-levels io.joynr.capabilities=FINEST && \
    asadmin --user admin --passwordfile=/opt/payara41/pwdfile set-log-levels io.joynr.arbitration=FINEST && \
    asadmin --user admin --passwordfile=/opt/payara41/pwdfile set-log-levels io.joynr.proxy=FINE && \
    asadmin --user admin --passwordfile=/opt/payara41/pwdfile set-log-levels io.joynr.jeeintegration=FINEST && \
    asadmin --user admin --passwordfile=/opt/payara41/pwdfile set-log-attributes com.sun.enterprise.server.logging.GFFileHandler.multiLineMode=false && \
    asadmin --user admin --passwordfile=/opt/payara41/pwdfile stop-domain || \
    true

ENTRYPOINT ["/start-me-up.sh"]
