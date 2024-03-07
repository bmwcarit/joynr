FROM alpine:3.13.6

LABEL com.jfrog.artifactory.retention.maxCount="25"

# curl may not be used by some implementations
# but it should not be a problem
RUN apk add --no-cache openjdk11
RUN apk add --no-cache curl
