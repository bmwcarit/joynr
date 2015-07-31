FROM joynr-base:latest

###################################################
# deploy the build scripts
###################################################
RUN mkdir /data/scripts/build
COPY scripts/build/* /data/scripts/build/
RUN chmod 777 -R /data/scripts/build/