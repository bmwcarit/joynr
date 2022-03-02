FROM joynr-android:latest

###################################################
# install node.js
###################################################
# nvm environment variables
ENV NVM_DIR /usr/local/nvm

ENV NODE_V8 8.16.2
ENV NODE_V12 12.22.2

# install nvm
RUN . /etc/profile \
    && mkdir -p $NVM_DIR \
    && curl --silent -o- https://raw.githubusercontent.com/creationix/nvm/v0.39.1/install.sh | bash

# install node and npm
# having the nvm directory writable makes it possible to use nvm to change node versions manually
# nvm uses curl internally with '-q' option suppressing evaluation of '.curlrc' hence
# if a proxy is set it is required to wrap curl to explicitly set a config file because
# nvm does not provide an option for this.
RUN . /etc/profile \
    && if [ -n "$PROXY_HOST" ]; then alias curl="/usr/bin/curl -K /etc/.curlrc"; fi \
    && source $NVM_DIR/nvm.sh \
    && nvm install $NODE_V12 \
    && nvm install $NODE_V8 \
    && nvm alias default $NODE_V8 \
    && nvm use default \
    && chmod -R a+rwx $NVM_DIR

# add node and npm to path
# (node will be available then without sourcing $NVM_DIR/nvm.sh)
ENV PATH $NVM_DIR/versions/node/v$NODE_V8/bin:$PATH

###################################################
# install chromium to run js tests with
# chromium headless
###################################################
RUN dnf install -y \
	chromium \
    && dnf clean all

###################################################
# deploy the build scripts
###################################################
COPY scripts/build/* /data/scripts/build/
RUN chmod 777 -R /data/scripts/build/
