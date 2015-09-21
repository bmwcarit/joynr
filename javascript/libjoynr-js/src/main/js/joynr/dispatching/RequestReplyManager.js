/*jslint es5: true */

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

define(
        "joynr/dispatching/RequestReplyManager",
        [
            "global/Promise",
            "joynr/dispatching/types/Reply",
            "joynr/messaging/MessagingQos",
            "joynr/messaging/inprocess/InProcessAddress",
            "joynr/TypesEnum",
            "joynr/util/Typing",
            "joynr/util/UtilInternal",
            "joynr/util/JSONSerializer",
            "joynr/util/LongTimer",
            "joynr/exceptions/MethodInvocationException",
            "joynr/exceptions/ProviderRuntimeException",
            "joynr/exceptions/ApplicationException",
            "joynr/system/LoggerFactory"
        ],
        function(
                Promise,
                Reply,
                MessagingQos,
                InProcessAddress,
                TypesEnum,
                Typing,
                Util,
                JSONSerializer,
                LongTimer,
                MethodInvocationException,
                ProviderRuntimeException,
                ApplicationException,
                LoggerFactory) {

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
             * @param {TypeRegistry}
             *            typeRegistry - the global type registry that records the type names
             *            together with their constructor.
             */
            function RequestReplyManager(dispatcher, typeRegistry) {
                var log = LoggerFactory.getLogger("joynr.dispatching.RequestReplyManager");

                var providers = {};
                var replyCallers = {};
                //var deletedReplyCallers = {};

                /**
                 * @name RequestReplyManager#sendRequest
                 * @function
                 *
                 * @param {Object}
                 *            settings
                 * @param {String}
                 *            settings.from participantId of the sender
                 * @param {String}
                 *            settings.to participantId of the receiver
                 * @param {MessagingQos}
                 *            settings.messagingQos quality-of-service parameters such as time-to-live
                 * @param {Request}
                 *            settings.request the Request to send
                 * @returns {Promise} the Promise for the Request
                 */
                this.sendRequest = function sendRequest(settings) {
                    var addReplyCaller = this.addReplyCaller;
                    return new Promise(function(resolve, reject) {
                        addReplyCaller(settings.request.requestReplyId, {
                            resolve : resolve,
                            reject : reject
                        }, settings.messagingQos.ttl);
                        // resolve will be called upon successful response
                        dispatcher.sendRequest(settings).catch(reject);
                    });
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
                    replyCallers[requestReplyId] = replyCaller;
                    LongTimer.setTimeout(function replyCallMissed() {
                        var replyCaller = replyCallers[requestReplyId];
                        if (replyCaller === undefined) {
                            return;
                        }
                        replyCaller.reject(new Error("Request with id \"" + requestReplyId + "\" failed: ttl expired"));
                        // remove the replyCaller from replyCallers in
                        // ttl_ms
                        // deletedReplyCallers[requestReplyId] = replyCaller;
                        delete replyCallers[requestReplyId];
                    }, ttl_ms);
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
                this.removeRequestCaller =
                        function removeRequestCaller(participantId) {
                            try {
                                delete providers[participantId];
                            } catch (error) {
                                log.error("error removing provider with participantId: "
                                    + participantId
                                    + " error: "
                                    + error);
                            }
                        };

                /**
                 * @name RequestReplyManager#handleRequest
                 * @function
                 *
                 * @param {Request}
                 *            request
                 */
                this.handleRequest =
                        function handleRequest(providerParticipantId, request, callbackDispatcher) {
                            var provider = providers[providerParticipantId];
                            var exception;
                            if (!provider) {
                                // TODO error handling request
                                // TODO what if no provider is found in the mean time?
                                // Do we need to add a task to handleRequest later?
                                exception = new MethodInvocationException({
                                    detailMessage: "error handling request: "
                                        + JSONSerializer.stringify(request)
                                        + " for providerParticipantId "
                                        + providerParticipantId
                                });
                                callbackDispatcher(new Reply({
                                    error : exception,
                                    requestReplyId : request.requestReplyId
                                }));
                                return;
                            }

                            // augment the type information
                            //var typedArgs = Typing.augmentTypes(request.params, typeRegistry);

                            // if there's an operation available to call
                            var result;
                            if (provider[request.methodName]
                                && provider[request.methodName].callOperation) {
                                // may throw an immediate exception when callOperation checks the
                                // arguments, in this case exception must be caught.
                                // If final customer provided method implementation gets called,
                                // that one may return either promise (preferred) or direct result
                                // and may possibly also throw exception in the latter case.
                                try {
                                    result =
                                        provider[request.methodName].callOperation(
                                            request.params,
                                            request.paramDatatypes);
                                } catch(internalException) {
                                    exception = internalException;
                                }
                            // otherwise, check whether request is an attribute get, set or an operation
                            } else {
                                var match = request.methodName.match(/([gs]et)?(\w+)/);
                                var getSet = match[1];
                                if (getSet) {
                                    var attributeName = Util.firstLower(match[2]);
                                    // if the attribute exists in the provider
                                    if (provider[attributeName]
                                        && !provider[attributeName].callOperation) {
                                        if (getSet === "get") {
                                            //TODO: assumes the getter is not throwing exception
                                            result = provider[attributeName].get();
                                        } else if (getSet === "set") {
                                            //TODO: assumes the setter is not throwing exception
                                            provider[attributeName].set(request.params[0]);
                                        }
                                    }
                                    // if neither an operation nor an attribute exists in the
                                    // provider => deliver MethodInvocationException
                                    else {
                                        exception = new MethodInvocationException({
                                            detailMessage: "Could not find an operation \""
                                                + request.methodName
                                                + "\" or an attribute \""
                                                + attributeName
                                                + "\" in the provider"
                                        });
                                    }
                                }
                                // if no operation was found and methodName didn't start with "get"
                                // or "set" => deliver MethodInvocationException
                                else {
                                    exception = new MethodInvocationException({
                                        detailMessage: "Could not find an operation \""
                                            + request.methodName + "\" in the provider"
                                    });
                                }
                            }

                            /* Asynchronously pass the result back to the dispatcher
                             *
                             * Call operations can be a sync or async method. In the sync case,
                             * the return value of the call operation
                             * is simply the result of the call. In the async case, the provider has
                             * the possibility to return a promise
                             * object. In this case, we wait until the promise object is resolved
                             * and call then the callbackDispatcher
                             */
                            if (!exception && Util.isPromise(result)) {
                                result.then(function(value) {
                                    callbackDispatcher(new Reply({
                                        response : [value],
                                        requestReplyId : request.requestReplyId
                                    }));
                                }).catch(function(internalException) {
                                    callbackDispatcher(new Reply({
                                        error : internalException,
                                        requestReplyId : request.requestReplyId
                                    }));
                                });
                            } else {
                                if (exception) {
                                    // return stored exception
                                    LongTimer.setTimeout(function asyncCallbackDispatcher() {
                                        callbackDispatcher(new Reply({
                                            error : exception,
                                            requestReplyId : request.requestReplyId
                                        }));
                                    }, 0);
                                } else {
                                    // return result of call
                                    LongTimer.setTimeout(function asyncCallbackDispatcher() {
                                        callbackDispatcher(new Reply({
                                            response : [result],
                                            requestReplyId : request.requestReplyId
                                        }));
                                    }, 0);
                                }
                            }
                        };

                /**
                 * @name RequestReplyManager#handleReply
                 * @function
                 *
                 * @param {Reply}
                 *            reply
                 */
                this.handleReply =
                        function handleReply(reply) {
                            var replyCaller = replyCallers[reply.requestReplyId];

                            if (replyCaller === undefined) {
                                log
                                        .error("error handling reply resolve, because replyCaller could not be found: "
                                            + JSONSerializer.stringify(reply, undefined, 4));
                                return;
                            }

                            try {
                                if (reply.error) {
                                    replyCaller.reject(reply.error);
                                } else {
                                    replyCaller.resolve(reply.response);
                                }
                                // deletedReplyCallers[reply.requestReplyId] = replyCallers[reply.requestReplyId];
                                delete replyCallers[reply.requestReplyId];
                            } catch (e) {
                                log.error("exception thrown during handling reply "
                                    + JSONSerializer.stringify(reply, undefined, 4)
                                    + ":\n"
                                    + e.stack);
                            }
                        };
            }

            return RequestReplyManager;

        });
