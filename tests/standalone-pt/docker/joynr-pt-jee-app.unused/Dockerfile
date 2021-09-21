FROM payara/server-full:5.2021.7-jdk11

USER root

LABEL com.jfrog.artifactory.retention.maxCount="25"

RUN apt-get update \
    && apt-get install -y --no-install-recommends curl

USER payara

COPY pt-jee-app.war /pt-jee-app.war
COPY start-me-up.sh /start-me-up.sh

ENV PATH ${PATH}:/opt/payara/appserver/bin

RUN asadmin start-domain && \
    asadmin --user admin --passwordfile=/opt/payara/passwordfile create-managed-scheduled-executor-service --corepoolsize=100 concurrent/joynrMessagingScheduledExecutor && \
    asadmin --user admin --passwordfile=/opt/payara/passwordfile set-log-levels io.joynr=SEVERE && \
    asadmin --user admin --passwordfile=/opt/payara/passwordfile set-log-levels com.hivemq=SEVERE && \
    true

ENTRYPOINT ["/start-me-up.sh"]
