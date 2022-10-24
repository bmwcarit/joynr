FROM payara/server-full:5.2021.7-jdk11

USER root

LABEL com.jfrog.artifactory.retention.maxCount="25"

RUN rm /etc/apt/sources.list.d/zulu-openjdk.list \
    && apt-get update \
    && apt-get install -y --no-install-recommends curl

USER payara

COPY target/discovery-directory-pt-jee.war /discovery-directory-pt-jee.war
COPY start-me-up.sh /start-me-up.sh

ENV PATH ${PATH}:/opt/payara/appserver/bin
ENV joynr_messaging_mqtt_brokeruris tcp://mqttbroker:1883
ENV joynr_gcd_gbid joynrdefaultgbid
ENV joynr_gcd_valid_gbids joynrdefaultgbid,othergbid
ENV joynr_messaging_gbids joynrdefaultgbid
ENV joynr_servlet_hostpath http://jee-provider:8080

RUN asadmin --user admin --passwordfile=/opt/payara/passwordFile start-domain && \
    asadmin --user admin --passwordfile=/opt/payara/passwordFile create-managed-scheduled-executor-service --corepoolsize=100 concurrent/joynrMessagingScheduledExecutor && \
    asadmin --user admin --passwordfile=/opt/payara/passwordFile set-log-levels io.joynr=SEVERE && \
    asadmin --user admin --passwordfile=/opt/payara/passwordFile set-log-levels com.hivemq=SEVERE && \
    asadmin --user admin --passwordfile=/opt/payara/passwordFile set-log-attributes com.sun.enterprise.server.logging.GFFileHandler.multiLineMode=false && \
    asadmin --user admin --passwordfile=/opt/payara/passwordFile stop-domain && \
    true

ENTRYPOINT ["/start-me-up.sh"]
