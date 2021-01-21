FROM payaradocker/payaraserver:4.1.1.161.1

COPY pt-jee-app.war /pt-jee-app.war
COPY start-me-up.sh /start-me-up.sh

ENV PATH ${PATH}:/opt/payara41/glassfish/bin
RUN echo 'AS_ADMIN_PASSWORD=glassfish\n\
EOF\n' \
>> /opt/payara41/pwdfile

RUN asadmin start-domain && \
    asadmin --user admin --passwordfile=/opt/payara41/pwdfile create-managed-scheduled-executor-service --corepoolsize=100 concurrent/joynrMessagingScheduledExecutor && \
    asadmin --user admin --passwordfile=/opt/payara41/pwdfile set-log-levels io.joynr=SEVERE && \
    asadmin --user admin --passwordfile=/opt/payara41/pwdfile set-log-levels com.hivemq=SEVERE && \
    true

ENTRYPOINT ["/start-me-up.sh"]
