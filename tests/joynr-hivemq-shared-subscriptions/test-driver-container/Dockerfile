FROM alpine:3.13.6

LABEL com.jfrog.artifactory.retention.maxCount="25"

RUN sed -i 's/https/http/g' /etc/apk/repositories

RUN apk add --no-cache bash curl grep

COPY start-me-up.sh /start-me-up.sh

ENTRYPOINT ["bash","/start-me-up.sh"]
