FROM payara/server-full:4.181

COPY target/discovery-directory-jee.war /discovery-directory-jee.war
COPY start-me-up.sh /start-me-up.sh

RUN ls /opt/payara41/glassfish/bin

ENV PATH ${PATH}:/opt/payara41/glassfish/bin
ENV joynr_messaging_mqtt_brokeruris tcp://mqttbroker:1883
ENV joynr_gcd_gbid joynrdefaultgbid
ENV joynr_gcd_valid_gbids joynrdefaultgbid
ENV joynr_messaging_gbids joynrdefaultgbid
ENV joynr_servlet_hostpath http://joynrbackend:8080

RUN echo 'AS_ADMIN_PASSWORD=admin\n\
EOF\n' \
>> /tmp/pwdfile
RUN asadmin start-domain && \
	asadmin --user admin --passwordfile=/tmp/pwdfile create-managed-scheduled-executor-service --corepoolsize=100 concurrent/joynrMessagingScheduledExecutor && \
	asadmin --user admin --passwordfile=/tmp/pwdfile create-jdbc-connection-pool --datasourceclassname org.apache.derby.jdbc.ClientDataSource --restype javax.sql.XADataSource --property portNumber=1527:password=APP:user=APP:serverName=localhost:databaseName=joynr-discovery-directory:connectionAttributes=\;create\\=true JoynrPool && \
	asadmin --user admin --passwordfile=/tmp/pwdfile create-jdbc-resource --connectionpoolid JoynrPool joynr/DiscoveryDirectoryDS && \
	asadmin --user admin --passwordfile=/tmp/pwdfile create-jdbc-resource --connectionpoolid JoynrPool joynr/DomainAccessControllerDS && \
	asadmin --user admin --passwordfile=/tmp/pwdfile start-database --jvmoptions="-Dderby.storage.useDefaultFilePermissions=true" && \
	asadmin --user admin --passwordfile=/tmp/pwdfile healthcheck-configure --enabled=true --dynamic=true && \
	asadmin --user admin --passwordfile=/tmp/pwdfile deploy /discovery-directory-jee.war && \
	asadmin --user admin --passwordfile=/tmp/pwdfile set-log-attributes com.sun.enterprise.server.logging.GFFileHandler.multiLineMode=false && \
	asadmin --user admin --passwordfile=/tmp/pwdfile set-log-levels io.joynr.messaging=FINE && \
	asadmin --user admin --passwordfile=/tmp/pwdfile set-log-levels io.joynr.dispatching=FINE && \
	asadmin --user admin --passwordfile=/tmp/pwdfile set-log-levels io.joynr.discovery=FINEST && \
	asadmin --user admin --passwordfile=/tmp/pwdfile set-log-levels io.joynr.capabilities=FINEST && \
	asadmin --user admin --passwordfile=/tmp/pwdfile set-log-levels io.joynr.arbitration=FINEST && \
	asadmin --user admin --passwordfile=/tmp/pwdfile set-log-levels io.joynr.proxy=FINE && \
	asadmin --user admin --passwordfile=/tmp/pwdfile set-log-levels io.joynr.jeeintegration=FINEST || \
	true

ENTRYPOINT ["/start-me-up.sh"]
