FROM payaradocker/payaraserver:4.1.1.161.1

LABEL com.jfrog.artifactory.retention.maxCount="25"

COPY target/discovery-directory-pt-jee.war /discovery-directory-pt-jee.war
COPY start-me-up.sh /start-me-up.sh

ENV PATH ${PATH}:/opt/payara41/glassfish/bin
ENV joynr_messaging_mqtt_brokeruris tcp://mqttbroker:1883
ENV joynr_gcd_gbid joynrdefaultgbid
ENV joynr_gcd_valid_gbids joynrdefaultgbid,othergbid
ENV joynr_messaging_gbids joynrdefaultgbid
ENV joynr_servlet_hostpath http://jee-provider:8080

RUN echo 'AS_ADMIN_PASSWORD=glassfish\n\
EOF\n' \
>> /opt/payara41/pwdfile
RUN asadmin --user admin --passwordfile=/opt/payara41/pwdfile start-domain && \
    asadmin --user admin --passwordfile=/opt/payara41/pwdfile create-managed-scheduled-executor-service --corepoolsize=100 concurrent/joynrMessagingScheduledExecutor && \
    asadmin --user admin --passwordfile=/opt/payara41/pwdfile create-jdbc-connection-pool --datasourceclassname org.apache.derby.jdbc.ClientDataSource --restype javax.sql.XADataSource --property portNumber=1527:password=APP:user=APP:serverName=infra-db:databaseName=joynr-discovery-directory:connectionAttributes=\;create\\=true JoynrPool && \
    asadmin --user admin --passwordfile=/opt/payara41/pwdfile create-jdbc-resource --connectionpoolid JoynrPool joynr/DiscoveryDirectoryDS && \
    asadmin --user admin --passwordfile=/opt/payara41/pwdfile set-log-levels io.joynr=SEVERE && \
    asadmin --user admin --passwordfile=/opt/payara41/pwdfile set-log-levels com.hivemq=SEVERE && \
    asadmin --user admin --passwordfile=/opt/payara41/pwdfile set-log-attributes com.sun.enterprise.server.logging.GFFileHandler.multiLineMode=false && \
    asadmin --user admin --passwordfile=/opt/payara41/pwdfile stop-domain && \
    true

ENTRYPOINT ["/start-me-up.sh"]
