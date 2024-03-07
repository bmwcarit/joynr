FROM java-11-with-curl:latest

RUN sed -i 's/https/http/g' /etc/apk/repositories

RUN apk add --no-cache bash grep

COPY start-me-up.sh /start-me-up.sh

ENTRYPOINT ["bash","/start-me-up.sh"]
