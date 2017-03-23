/*jslint es5: true */

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

define(
        "joynr/messaging/channel/LongPollingChannelMessageReceiver",
        [
            "global/Promise",
            "uuid",
            "joynr/messaging/JoynrMessage",
            "JsonParser",
            "joynr/messaging/MessagingQos",
            "joynr/system/DiagnosticTags",
            "joynr/system/LoggerFactory",
            "joynr/util/UtilInternal",
            "joynr/util/LongTimer"
        ],
        function(
                Promise,
                uuid,
                JoynrMessage,
                JsonParser,
                MessagingQos,
                DiagnosticTags,
                LoggerFactory,
                Util,
                LongTimer) {

            /**
             * LongPollingChannelMessageReceiver handles the long poll to the bounce proxy.
             *
             * @name LongPollingChannelMessageReceiver
             * @constructor
             *
             * @param {Object}
             *            settings an object containing required input for LongPollingChannelMessageReceiver
             * @param {String}
             *            settings.bounceProxyUrl an object describing the bounce proxy URL
             * @param {CommunicationModule}
             *            settings.communicationModule the API required by the LongPollingChannelMessageReceiver to perform XMLHTTPRequests
             *            socket communication
             * @param {Object}
             *            settings.channelQos parameters specifying the qos requirements for the channel (creation)
             * @param {Number}
             *            settings.channelQos.creationTimeout_ms max time to create a channel
             * @param {Number}
             *            settings.channelQos.creationRetryDelay_ms retry interval after a failing channel creation
             *
             * @returns the LongPollingChannelMessageReceiver
             */
            function LongPollingChannelMessageReceiver(settings) {
                var storagePrefix = "joynr.channels";
                var persistency = settings.persistency;
                var onReceive;
                var channelOpMessagingQos = new MessagingQos();
                var channelId, channelUrl;
                var communicationModule = settings.communicationModule;
                var log = LoggerFactory.getLogger("joynr.messaging.LongPollingChannelMessageReceiver");
                var bounceProxyChannelBaseUrl = settings.bounceProxyUrl + "channels/";
                var channelCreationTimeout_ms = settings.channelQos && settings.channelQos.creationTimeout_ms ? settings.channelQos.creationTimeout_ms : 1000*60*60*24; // default: 1 day

                var channelCreationRetryDelay_ms = settings.channelQos && settings.channelQos.creationRetryDelay_ms ? settings.channelQos.creationRetryDelay_ms : 1000*30; // default: 30s
                var createChannelTimestamp;
                /**
                 * Retrieve the receiverId for the given channelId
                 *
                 * @name LongPollingChannelMessageReceiver#getReceiverId
                 * @function
                 * @private
                 */
                function getReceiverId(channelId) {
                    var receiverId =
                            persistency.getItem(storagePrefix + "." + channelId + ".receiverId");
                    if (receiverId === undefined || receiverId === null) {
                        receiverId = "tid-" + uuid();
                        persistency.setItem(
                                storagePrefix + "." + channelId + ".receiverId",
                                receiverId);
                    }
                    return receiverId;
                }

                /**
                 * Stop long polling
                 *
                 * @name LongPollingChannelMessageReceiver#stop
                 * @function
                 */
                this.stop = function stop() {
                    communicationModule.atmosphere.unsubscribeUrl(channelUrl);
                };

                /**
                 * Removes the channel with a given channel id
                 *
                 * @name LongPollingChannelMessageReceiver#clear
                 * @function
                 *
                 * @returns {Object} a promise object for async event handling
                 */
                this.clear =
                        function clear() {
                            log.debug("clearing channel with id " + channelId + " and url " + channelUrl);
                            return communicationModule.createXMLHTTPRequest({
                                type : "DELETE",
                                url : channelUrl,
                                timeout : channelOpMessagingQos.ttl
                            }).then(function() {
                                return;
                            }).catch(function(xhr, errorType) {
                                var errorString =
                                        "error while deleting channel on bounce proxy: "
                                            + errorType
                                            + (xhr.statusText ? (", " + xhr.statusText) : "")
                                            + (xhr.status === 0 ? "" : (", HTTP" + xhr.status))
                                            + (xhr.responseText ? (", " + xhr.responseText) : "");
                                log.error(
                                        "error while deleting channel on bounce proxy",
                                        JSON.stringify(DiagnosticTags.forChannel({
                                            channelId : channelId,
                                            channelUrl : channelUrl,
                                            status : xhr.status,
                                            responseText : xhr.responseText
                                        })));
                                throw new Error(errorString);
                            });
                        };

                /**
                 * Start long polling. Side effect: first makes sure that the channel exists on the bounceproxy
                 *
                 * @name LongPollingChannelMessageReceiver#start
                 * @function
                 *
                 * @param {Function}
                 *            onMessageCallback callback used to pass the received messages up the processing chain
                 * @param {Function}
                 *            onError callback used to inform about errors occurred during message recieve
                 */
                this.start =
                        function start(onMessageCallback, onError) {
                            var pollRequest;
                            var receiverId = getReceiverId(channelId);

                            communicationModule.atmosphere.unsubscribeUrl(channelUrl);

                            pollRequest = {
                                url : channelUrl,
                                transport : "long-polling",
                                contentType : "application/json",
                                timeout : 30000,
                                enableXDR : false,
                                enableProtocol : false,
                                readResponseHeaders : false,
                                attachHeadersAsQueryString : false,
                                dropAtmosphereHeaders : true,
                                dropHeaders : false,
                                logLevel : "debug",
                                headers : {
                                    "X-Atmosphere-tracking-id" : receiverId
                                }

                            };

                            // Called whenever one or more new
                            // messages are received
                            // from the bounceproxy.
                            pollRequest.onMessage =
                                    function onMessage(response) {
                                        var detectedTransport = response.transport, data, jsonParser, joynrMessage;

                                        try {
                                            if (response.status === 200) {
                                                data = response.responseBody;

                                                if (data.length > 0) {
                                                    jsonParser = new JsonParser(data);
                                                    while (jsonParser.hasNext) {
                                                        // pass the message on
                                                        joynrMessage = new JoynrMessage(jsonParser.next);

                                                        log
                                                                .info(
                                                                        "received message with id " + joynrMessage.msgId + ": ",
                                                                        JSON
                                                                                .stringify(DiagnosticTags
                                                                                        .forJoynrMessage(joynrMessage)));
                                                        onMessageCallback(joynrMessage);
                                                    }
                                                }
                                            }
                                        } catch (e) {
                                            log.debug("Exception while processing message: "
                                                + e.message);
                                            if (onError) {
                                                onError(e);
                                            }
                                        }
                                    };

                            pollRequest.onError = function(response) {
                                log.debug("Encountered atmosphere error :" + response.error);
                                if (onError) {
                                    onError(response.error);
                                }
                            };

                            communicationModule.atmosphere.subscribe(pollRequest);

                            log.debug("starting to listen on channel with id "
                                + channelId
                                + " and url "
                                + channelUrl);
                        };

                var callCreate = function callCreate(){
                    var createChannelUrl =
                        bounceProxyChannelBaseUrl
                        + "?ccid="
                        + encodeURIComponent(channelId);
                    var receiverId = getReceiverId(channelId);
                    return communicationModule.createXMLHTTPRequest({
                        type : "POST",
                        url : createChannelUrl,
                        headers : {
                            "X-Atmosphere-tracking-id" : receiverId
                        },
                        timeout : channelOpMessagingQos.ttl
                    }).then(function(xhr) {
                        channelUrl = xhr.getResponseHeader("Location");

                        if (!channelUrl) {
                            channelUrl = xhr.responseText;
                        }

                        log.debug("created channel with id "
                                + channelId
                                + " and url "
                                + channelUrl);

                        return channelUrl;
                    });
                };

                function logChannelCreationError(xhr){
                    log.error(
                            "error while creating channel on bounce proxy",
                            {
                                channelId : channelId,
                                channelUrl : channelUrl,
                                status : xhr.status,
                                responseText : xhr.responseText
                            });
                }

                var createInternal = function createInternal(resolve, reject) {
                    callCreate().then(resolve).catch(function(xhr, errorType) {
                        logChannelCreationError(xhr);
                        if (createChannelTimestamp + channelCreationTimeout_ms <= Date.now()) {
                            reject(new Error("Error creating channel"));
                        } else {
                            LongTimer.setTimeout(createInternal, channelCreationRetryDelay_ms, resolve, reject);
                        }
                    });
                };

                /**
                 * Create the given Channel on the bounceproxy
                 *
                 * @name LongPollingChannelMessageReceiver#create
                 * @function
                 *
                 * @param {String}
                 *            channelId the channelId that will be created, will be URI-Component-encoded
                 * @returns {Object} a promise object for async event handling
                 */
                this.create = function create(theChannelId) {
                    if (channelId !== undefined) {
                        throw new Error("LongPollingChannelMessageReceiver.create has already been called for channelId: " + theChannelId);
                    }
                    channelId = theChannelId;
                    createChannelTimestamp = Date.now();
                    return callCreate().catch(function(xhr, errorType) {
                        logChannelCreationError(xhr);
                        if (createChannelTimestamp + channelCreationTimeout_ms <= Date.now()) {
                            throw new Error("Error creating channel");
                        } else {
                            return new Promise(function(resolve, reject) {
                                LongTimer.setTimeout(createInternal, channelCreationRetryDelay_ms, resolve, reject);
                            });
                        }
                    });
                };

                /**
                 * Parse incoming JSON
                 *
                 * @name LongPollingChannelMessageReceiver#parse
                 * @function
                 * @private
                 */
                function parse(data) {
                    if (window.JSON && window.JSON.parse) {
                        try {
                            return window.JSON.parse(data);
                        } catch (err) {
                            return data;
                        }
                    }
                }

            }
            return LongPollingChannelMessageReceiver;
        });