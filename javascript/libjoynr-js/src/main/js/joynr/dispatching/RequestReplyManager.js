/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
 * %%
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * #L%
 */
const Reply = require("./types/Reply");
const Typing = require("../util/Typing");
const UtilInternal = require("../util/UtilInternal");
const JSONSerializer = require("../util/JSONSerializer");
const MethodInvocationException = require("../exceptions/MethodInvocationException");
const ProviderRuntimeException = require("../exceptions/ProviderRuntimeException");
const Version = require("../../generated/joynr/types/Version");
const LoggingManager = require("../system/LoggingManager");
const util = require("util");
/**
 * The RequestReplyManager is responsible maintaining a list of providers that wish to
 * receive incoming requests, and also a list of requestReplyIds which is used to match
 * an incoming message with an expected reply.
 *
 * @name RequestReplyManager
 * @constructor
 *
 * @param {Dispatcher}
 *            dispatcher
 */
function RequestReplyManager(dispatcher) {
    const log = LoggingManager.getLogger("joynr.dispatching.RequestReplyManager");

    const providers = {};
    const replyCallers = new Map();
    let started = true;

    const CLEANUP_CYCLE_INTERVAL = 1000;

    const cleanupInterval = setInterval(() => {
        const currentTime = Date.now();
        for (const [id, caller] of replyCallers) {
            if (caller.expiresAt <= currentTime) {
                caller.callback(new Error(`Request with id "${id}" failed: ttl expired`));
                replyCallers.delete(id);
            }
        }
    }, CLEANUP_CYCLE_INTERVAL);

    function checkIfReady() {
        if (!started) {
            throw new Error("RequestReplyManager is already shut down");
        }
    }

    function sendRequestInternal(settings, callbackSettings, callback) {
        try {
            checkIfReady();
        } catch (e) {
            callback(e);
        }

        this.addReplyCaller(
            settings.request.requestReplyId,
            {
                callback,
                callbackSettings
            },
            settings.messagingQos.ttl
        );

        dispatcher.sendRequest(settings);
    }

    /**
     * @name RequestReplyManager#sendRequest
     * @function
     *
     * @param {Object}
     *            settings
     * @param {String}
     *            settings.from participantId of the sender
     * @param {DiscoveryEntryWithMetaInfo}
     *            settings.toDiscoveryEntry DiscoveryEntry of the receiver
     * @param {MessagingQos}
     *            settings.messagingQos quality-of-service parameters such as time-to-live
     * @param {Request}
     *            settings.request the Request to send
     * @param {Object} callbackSettings
     *          additional settings to handle the reply.
     * @returns {Promise} the Promise for the Request
     */
    this.sendRequest = util.promisify(sendRequestInternal);

    /**
     * @name RequestReplyManager#sendOneWayRequest
     * @function
     *
     * @param {Object}
     *            settings
     * @param {String}
     *            settings.from participantId of the sender
     * @param {DiscoveryEntryWithMetaInfo}
     *            settings.toDiscoveryEntry DiscoveryEntry of the receiver
     * @param {MessagingQos}
     *            settings.messagingQos quality-of-service parameters such as time-to-live
     * @param {OneWayRequest}
     *            settings.request the Request to send
     * @returns {Promise} the Promise for the Request
     */
    this.sendOneWayRequest = function sendOneWayRequest(settings) {
        checkIfReady();
        return dispatcher.sendOneWayRequest(settings);
    };

    /**
     * The function addRequestCaller is called when a provider wishes to receive
     * incoming requests for the given participantId
     *
     * @name RequestReplyManager#addRequestCaller
     * @function
     *
     * @param {String}
     *            participantId of the provider receiving the incoming requests
     * @param {Provider}
     *            provider
     */
    this.addRequestCaller = function addRequestCaller(participantId, provider) {
        checkIfReady();
        providers[participantId] = provider;
    };

    /**
     * The function addReplyCaller is called when a proxy get/set or rpc is called and
     * is waiting for a reply The reply caller is automatically
     * removed when the ttl expires.
     *
     * @name RequestReplyManager#addReplyCaller
     * @function
     *
     * @param {String}
     *            requestReplyId
     * @param {ReplyCaller}
     *            replyCaller
     * @param {Number}
     *            ttl_ms relative number of milliseconds to wait for the reply.
     *            The replycaller will be removed in ttl_ms and and Error will be passed
     *            to the replyCaller
     */
    this.addReplyCaller = function addReplyCaller(requestReplyId, replyCaller, ttl_ms) {
        checkIfReady();
        replyCaller.expiresAt = Date.now() + ttl_ms;
        replyCallers.set(requestReplyId, replyCaller);
    };

    /**
     * The function removeRequestCaller is called when a provider no longer wishes to
     * receive incoming requests
     *
     * @name RequestReplyManager#removeRequestCaller
     * @function
     *
     * @param {String}
     *            participantId
     */
    this.removeRequestCaller = function removeRequestCaller(participantId) {
        checkIfReady();
        try {
            delete providers[participantId];
        } catch (error) {
            log.error(`error removing provider with participantId: ${participantId} error: ${error}`);
        }
    };

    function createReplyFromError(exception, requestReplyId, handleReplyCallback, replySettings) {
        const reply = Reply.create({
            error: exception,
            requestReplyId
        });
        return handleReplyCallback(replySettings, reply);
    }

    /**
     * @name RequestReplyManager#handleRequest
     * @param {String} providerParticipantId
     * @param {Request} request
     * @param {Function} handleReplyCallback
     *          callback for handling the reply
     * @param {Object} replySettings
     *          settings for handleReplyCallback to avoid unnecessary function object creation
     * @returns {*}
     */
    this.handleRequest = async function handleRequest(
        providerParticipantId,
        request,
        handleReplyCallback,
        replySettings
    ) {
        let exception;

        try {
            checkIfReady();
        } catch (error) {
            exception = new MethodInvocationException({
                detailMessage: `error handling request: ${JSONSerializer.stringify(
                    request
                )} for providerParticipantId ${providerParticipantId}. Joynr runtime already shut down.`
            });
            return createReplyFromError(exception, request.requestReplyId, handleReplyCallback, replySettings);
        }
        const provider = providers[providerParticipantId];
        if (!provider) {
            // TODO error handling request
            // TODO what if no provider is found in the mean time?
            // Do we need to add a task to handleRequest later?
            exception = new MethodInvocationException({
                detailMessage: `error handling request: ${JSONSerializer.stringify(
                    request
                )} for providerParticipantId ${providerParticipantId}`
            });
            return createReplyFromError(exception, request.requestReplyId, handleReplyCallback, replySettings);
        }

        // if there's an operation available to call
        let result;
        if (provider[request.methodName] && provider[request.methodName].callOperation) {
            // may throw an immediate exception when callOperation checks the
            // arguments, in this case exception must be caught.
            // If final customer provided method implementation gets called,
            // that one may return either promise (preferred) or direct result
            // and may possibly also throw exception in the latter case.
            try {
                result = await provider[request.methodName].callOperation(request.params, request.paramDatatypes);
            } catch (internalException) {
                exception = internalException;
            }
            // otherwise, check whether request is an attribute get, set or an operation
        } else {
            const match = request.methodName.match(/([gs]et)?(\w+)/);
            const getSet = match[1];
            if (getSet) {
                const attributeName = match[2];
                const attributeObject = provider[attributeName] || provider[UtilInternal.firstLower(attributeName)];
                // if the attribute exists in the provider
                if (attributeObject && !attributeObject.callOperation) {
                    try {
                        if (getSet === "get") {
                            result = await attributeObject.get();
                        } else if (getSet === "set") {
                            result = await attributeObject.set(request.params[0]);
                        }
                    } catch (internalGetterSetterException) {
                        if (internalGetterSetterException instanceof ProviderRuntimeException) {
                            exception = internalGetterSetterException;
                        } else {
                            exception = new ProviderRuntimeException({
                                detailMessage: `getter/setter method of attribute ${attributeName} reported an error`
                            });
                        }
                    }
                } else {
                    // if neither an operation nor an attribute exists in the
                    // provider => deliver MethodInvocationException
                    exception = new MethodInvocationException({
                        detailMessage: `Could not find an operation "${
                            request.methodName
                        }" or an attribute "${attributeName}" in the provider`,
                        providerVersion: new Version({
                            majorVersion: provider.constructor.MAJOR_VERSION,
                            minorVersion: provider.constructor.MINOR_VERSION
                        })
                    });
                }
            } else {
                // if no operation was found and methodName didn't start with "get"
                // or "set" => deliver MethodInvocationException
                exception = new MethodInvocationException({
                    detailMessage: `Could not find an operation "${request.methodName}" in the provider`,
                    providerVersion: new Version({
                        majorVersion: provider.constructor.MAJOR_VERSION,
                        minorVersion: provider.constructor.MINOR_VERSION
                    })
                });
            }
        }

        /*
          both ProviderOperation.callOperation and ProviderAttribute.get/set have
          have asynchronous API. Therefore the result is always a promise and thus
          it's possible to await for its result.
        */

        if (exception) {
            return createReplyFromError(exception, request.requestReplyId, handleReplyCallback, replySettings);
        }

        const reply = Reply.create({
            response: result,
            requestReplyId: request.requestReplyId
        });
        return handleReplyCallback(replySettings, reply);
    };

    /**
     * @name RequestReplyManager#handleOneWayRequest
     * @function
     *
     * @param {String}
     *            providerParticipantId
     * @param {OneWayRequest}
     *            request
     */
    this.handleOneWayRequest = function handleOneWayRequest(providerParticipantId, request) {
        checkIfReady();
        const provider = providers[providerParticipantId];
        if (!provider) {
            throw new MethodInvocationException({
                detailMessage: `error handling one-way request: ${JSONSerializer.stringify(
                    request
                )} for providerParticipantId ${providerParticipantId}`
            });
        }

        // if there's an operation available to call
        if (provider[request.methodName] && provider[request.methodName].callOperation) {
            // If final customer provided method implementation gets called,
            // that one may return either promise (preferred) or direct result
            // and may possibly also throw exception in the latter case.
            provider[request.methodName].callOperation(request.params, request.paramDatatypes);
        } else {
            throw new MethodInvocationException({
                detailMessage: `Could not find an operation "${request.methodName}" in the provider`,
                providerVersion: new Version({
                    majorVersion: provider.constructor.MAJOR_VERSION,
                    minorVersion: provider.constructor.MINOR_VERSION
                })
            });
        }
    };

    /**
     * @name RequestReplyManager#handleReply
     * @function
     *
     * @param {Reply}
     *            reply
     */
    this.handleReply = function handleReply(reply) {
        const replyCaller = replyCallers.get(reply.requestReplyId);

        if (replyCaller === undefined) {
            log.error(
                `error handling reply resolve, because replyCaller could not be found: ${JSONSerializer.stringify(
                    reply,
                    undefined,
                    4
                )}`
            );
            return;
        }

        try {
            if (reply.error) {
                if (reply.error instanceof Error) {
                    replyCaller.callback(reply.error);
                } else {
                    replyCaller.callback(Typing.augmentTypes(reply.error));
                }
            } else {
                replyCaller.callback(undefined, {
                    response: reply.response,
                    settings: replyCaller.callbackSettings
                });
            }

            replyCallers.delete(reply.requestReplyId);
        } catch (e) {
            log.error(
                `exception thrown during handling reply ${JSONSerializer.stringify(reply, undefined, 4)}:\n${e.stack}`
            );
        }
    };

    /**
     * Shutdown the request reply manager
     *
     * @function
     * @name RequestReplyManager#shutdown
     */
    this.shutdown = function shutdown() {
        clearInterval(cleanupInterval);

        for (const replyCaller of replyCallers.values()) {
            if (replyCaller) {
                replyCaller.callback(new Error("RequestReplyManager is already shut down"));
            }
        }
        replyCallers.clear();
        started = false;
    };
}

module.exports = RequestReplyManager;
