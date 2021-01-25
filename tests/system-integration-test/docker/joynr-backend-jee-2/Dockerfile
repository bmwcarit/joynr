FROM payaradocker/payaraserver:4.1.1.161.1

LABEL com.jfrog.artifactory.retention.maxCount="25"

COPY target/discovery-directory-jee-shared-db.war /discovery-directory-jee-shared-db.war
COPY target/domain-access-controller-jee.war /domain-access-controller-jee.war
COPY start-me-up.sh /start-me-up.sh

ENV PATH ${PATH}:/opt/payara41/glassfish/bin
ENV joynr_messaging_mqtt_brokeruris tcp://mqttbroker-2:1883
ENV joynr_servlet_hostpath http://joynrbackend-2:8080
ENV joynr_gcd_gbid othergbid
ENV joynr_gcd_valid_gbids joynrdefaultgbid,othergbid
ENV joynr_messaging_gbids othergbid

RUN echo 'AS_ADMIN_PASSWORD=glassfish\n\
EOF\n' \
>> /opt/payara41/pwdfile
RUN asadmin start-domain && \
	asadmin --user admin --passwordfile=/opt/payara41/pwdfile create-managed-scheduled-executor-service --corepoolsize=100 concurrent/joynrMessagingScheduledExecutor && \
	asadmin --user admin --passwordfile=/opt/payara41/pwdfile create-jdbc-connection-pool --datasourceclassname org.apache.derby.jdbc.ClientDataSource --restype javax.sql.XADataSource --property portNumber=1527:password=APP:user=APP:serverName=joynrbackend-db:databaseName=joynr-discovery-directory:connectionAttributes=\;create\\=true JoynrPool && \
	asadmin --user admin --passwordfile=/opt/payara41/pwdfile create-jdbc-resource --connectionpoolid JoynrPool joynr/DiscoveryDirectoryDS && \
	asadmin --user admin --passwordfile=/opt/payara41/pwdfile create-jdbc-resource --connectionpoolid JoynrPool joynr/DomainAccessControllerDS && \
	asadmin --user admin --passwordfile=/opt/payara41/pwdfile set-log-levels io.joynr.messaging=FINEST && \
	asadmin --user admin --passwordfile=/opt/payara41/pwdfile set-log-levels io.joynr.dispatching=FINEST && \
	asadmin --user admin --passwordfile=/opt/payara41/pwdfile set-log-levels io.joynr.jeeintegration=FINEST || \
	true

ENTRYPOINT ["/start-me-up.sh"]
