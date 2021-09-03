FROM postgres:alpine

LABEL com.jfrog.artifactory.retention.maxCount="25"

ENV POSTGRES_USER gcd
ENV POSTGRES_PASSWORD gcd
ENV POSTGRES_DB gcd

COPY init.sql /docker-entrypoint-initdb.d/init.sql

CMD ["postgres", "-c", "log_statement=all"]

