/*global JSON: true */
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
        "joynr/messaging/routing/MessageRouter",
        [
            "global/Promise",
            "joynr/system/DiagnosticTags",
            "joynr/system/LoggerFactory",
            "joynr/messaging/JoynrMessage",
            "joynr/util/Typing",
            "joynr/util/JSONSerializer",
        ],
        function(Promise, DiagnosticTags, LoggerFactory, JoynrMessage, Typing, JSONSerializer) {

            /**
             * Message Router receives a message and forwards it to the correct endpoint, as looked up in the {@link RoutingTable}
             *
             * @constructor
             * @name MessageRouter
             *
             * @param {Object}
             *            settings the settings object holding dependencies
             * @param {RoutingTable}
             *            settings.routingTable
             * @param {MessagingStubFactory}
             *            settings.messagingStubFactory
             * @param {String}
             *            settings.myChannelId
             * @param {Address} settings.incomingAddress
             * @param {Address} settings.parentMessageRouterAddress
             * @param {LocalStorage} settings.persistency - LocalStorage or another object implementing the same interface
             * @param {TypeRegistry} settings.typeRegistry
             *
             * @classdesc The <code>MessageRouter</code> is a joynr internal interface. The Message
             * Router receives messages from Message Receivers, and forwards them along using to the
             * appropriate Address, either <code>{@link ChannelAddress}</code> for messages being
             * sent to a joynr channel via HTTP, <code>{@link BrowserAddress}</code> for messages
             * going to applications running within a seperate browser tab, or
             * <code>{@link InProcessAddress}</code> for messages going to a dispatcher running
             * within the same JavaScript scope as the MessageRouter.
             *
             * MessageRouter is part of the cluster controller, and is used for
             * internal messaging only.
             */
            function MessageRouter(settings) {
                // TODO remote provider participants are registered by capabilitiesDirectory on lookup
                // TODO local providers registered (using dispatcher as local endpoint) by capabilitiesDirectory on register
                // TODO local replyCallers are registered (using dispatcher as local endpoint) when they are created

                var that = this;
                var log = LoggerFactory.getLogger("joynr/messaging/routing/MessageRouter");
                var listener, routingProxy, messagingStub;
                var queuedAddNextHopCalls = [], queuedRemoveNextHopCalls = [];
                var routingTable = settings.initialRoutingTable || {};
                var id = settings.joynrInstanceId;
                var persistency = settings.persistency;
                var incomingAddress = settings.incomingAddress;
                var parentMessageRouterAddress = settings.parentMessageRouterAddress;
                var typeRegistry = settings.typeRegistry;

                // if (settings.routingTable === undefined) {
                // throw new Error("routing table is undefined");
                // }
                if (settings.parentMessageRouterAddress !== undefined
                    && settings.incomingAddress === undefined) {
                    throw new Error("incoming address is undefined");
                }
                if (settings.messagingStubFactory === undefined) {
                    throw new Error("messaging stub factory is undefined");
                }
                if (settings.messageQueue === undefined) {
                    throw new Error("messageQueue is undefined");
                }

                /**
                 * @function MessageRouter#getStorageKey
                 *
                 * @param {String} participantId
                 *
                 * @returns {String}
                 */
                this.getStorageKey = function getStorageKey(participantId) {
                    return id + "_" + participantId.replace("/", ".");
                };

                /**
                 * @function MessageRouter#removeNextHop
                 *
                 * @param {String} participantId
                 *
                 * @returns {Promise} promise
                 */
                this.removeNextHop = function removeNextHop(participantId) {
                    routingTable[participantId] = undefined;
                    persistency.removeItem(that.getStorageKey(participantId));
                    var promise;
                    if (routingProxy !== undefined) {
                        promise = routingProxy.removeNextHop({
                            participantId : participantId
                        });
                    } else if (parentMessageRouterAddress !== undefined) {
                        promise = new Promise(function(resolve, reject) {
                            queuedRemoveNextHopCalls[queuedRemoveNextHopCalls.length] = {
                                participantId : participantId,
                                resolve : resolve,
                                reject : reject
                            };
                        });
                    } else {
                        promise = Promise.resolve();
                    }
                    return promise;
                };

                /**
                 * @function MessageRouter#addNextHopToParentRoutingTable
                 *
                 * @param {String} participantId
                 *
                 * @returns result
                 */
                this.addNextHopToParentRoutingTable =
                        function addNextHopToParentRoutingTable(participantId) {
                            var result;
                            if (Typing.getObjectType(incomingAddress) === "BrowserAddress") {
                                result = routingProxy.addNextHop({
                                    participantId : participantId,
                                    browserAddress : incomingAddress
                                });
                            } else if (Typing.getObjectType(incomingAddress) === "ChannelAddress") {
                                result = routingProxy.addNextHop({
                                    participantId : participantId,
                                    channelAddress : incomingAddress
                                });
                            } else if (Typing.getObjectType(incomingAddress) === "WebSocketAddress") {
                                result = routingProxy.addNextHop({
                                    participantId : participantId,
                                    webSocketAddress : incomingAddress
                                });
                            } else if (Typing.getObjectType(incomingAddress) === "WebSocketClientAddress") {
                                result = routingProxy.addNextHop({
                                    participantId : participantId,
                                    webSocketClientAddress : incomingAddress
                                });
                            } else if (Typing.getObjectType(incomingAddress) === "CommonApiDbusAddress") {
                                result = routingProxy.addNextHop({
                                    participantId : participantId,
                                    commonApiDbusAddress : incomingAddress
                                });
                            }
                            return result;
                        };

                /**
                 * @function MessageRouter#setRoutingProxy
                 *
                 * @param {RoutingProxy} newRoutingproxy - the routing proxy to be set
                 */
                this.setRoutingProxy =
                        function setRoutingProxy(newRoutingProxy) {
                            var hop, participantId, errorFct;

                            errorFct = function(error) {
                                throw new Error(error);
                            };

                            routingProxy = newRoutingProxy;
                            if (routingProxy !== undefined) {
                                if (routingProxy.proxyParticipantId !== undefined) {
                                    that.addNextHopToParentRoutingTable(
                                            routingProxy.proxyParticipantId).catch(errorFct);
                                }
                                for (hop in queuedAddNextHopCalls) {
                                    if (queuedAddNextHopCalls.hasOwnProperty(hop)) {
                                        var queuedHopCall = queuedAddNextHopCalls[hop];
                                        if (queuedHopCall.participantId !== routingProxy.proxyParticipantId) {
                                            that.addNextHopToParentRoutingTable(
                                                    queuedHopCall.participantId).then(
                                                    queuedHopCall.resolve).catch(queuedHopCall.reject);
                                        }
                                    }
                                }
                                for (hop in queuedRemoveNextHopCalls) {
                                    if (queuedRemoveNextHopCalls.hasOwnProperty(hop)) {
                                        var queuedCall = queuedRemoveNextHopCalls[hop];
                                        that.removeNextHop(queuedCall.participantId).then(
                                                queuedCall.resolve).catch(queuedCall.reject);
                                    }
                                }
                            }
                            queuedAddNextHopCalls = undefined;
                            queuedRemoveNextHopCalls = undefined;
                        };

                /**
                 * Looks up an Address for a given participantId (next hop)
                 *
                 * @name MessageRouter#resolveNextHop
                 * @function
                 *
                 * @param {String}
                 *            participantId
                 * @returns {InProcessAddress|BrowserAddress|ChannelAddress} the address of the next hop in the direction of the given
                 *          participantId, or undefined if not found
                 */
                this.resolveNextHop = function resolveNextHop(participantId) {
                    var address, addressString;
                    address = routingTable[participantId];

                    if (address === undefined) {
                        addressString = persistency.getItem(that.getStorageKey(participantId));
                        if ((addressString === undefined
                            || addressString === null || addressString === '{}')) {
                            persistency.removeItem(that.getStorageKey(participantId));
                        } else {
                            address = Typing.augmentTypes(JSON.parse(addressString), typeRegistry);
                            routingTable[participantId] = address;
                        }
                    }
                    if (address === undefined && routingProxy !== undefined) {
                        return routingProxy.resolveNextHop({
                                participantId : participantId
                            }).then(function(opArgs) {
                                if (opArgs.resolved) {
                                    return parentMessageRouterAddress;
                                }
                                throw new Error("nextHop cannot be resolved, as participant with id "
                                                + participantId
                                                + " is not reachable by parent routing table");
                            });
                    }
                    return Promise.resolve(address);
                };

                /**
                 * @name MessageRouter#route
                 * @function
                 *
                 * @param {JoynrMessage}
                 *            joynrMessage
                 * @returns {Object} A+ promise object
                 */
                this.route =
                        function route(joynrMessage) {
                                return that.resolveNextHop(joynrMessage.to).then(function(address) {
                                        var errorMsg;
                                        // Error: The participant is not registered yet.
                                        // remote provider participants are registered by capabilitiesDirectory on lookup
                                        // local providers are registered by capabilitiesDirectory on register
                                        // replyCallers are registered when they are created
                                        if (address === undefined) {
                                            errorMsg =
                                                    "No message receiver found for participantId: "
                                                        + joynrMessage.to
                                                        + ". Queuing request message.";
                                            log.info(errorMsg, DiagnosticTags
                                                    .forJoynrMessage(joynrMessage));
                                            settings.messageQueue.putMessage(joynrMessage);
                                            throw new Error(errorMsg);
                                        } else {
                                            messagingStub =
                                                    settings.messagingStubFactory
                                                            .createMessagingStub(address);
                                            if (messagingStub === undefined) {
                                                errorMsg =
                                                        "No message receiver found for participantId: "
                                                            + joynrMessage.to
                                                            + " queuing message.";
                                                log.info(errorMsg, DiagnosticTags
                                                        .forJoynrMessage(joynrMessage));
                                                throw new Error(errorMsg);
                                            } else {
                                                return messagingStub.transmit(joynrMessage);
                                            }
                                        }
                            });
                        };

                /**
                 * Registers the next hop with this specific participant Id
                 *
                 * @name RoutingTable#addNextHop
                 * @function
                 *
                 * @param {String}
                 *            participantId
                 * @param {InProcessAddress|BrowserAddress|ChannelAddress}
                 *            address the address to register
                 */
                this.addNextHop =
                        function addNextHop(participantId, address) {
                            // store the address of the participantId persistently
                            routingTable[participantId] = address;
                            var serializedAddress = JSONSerializer.stringify(address);
                            var promise;
                            if ((serializedAddress === undefined
                                || serializedAddress === null || serializedAddress === '{}')) {
                                log.info("addNextHop: HOP address "
                                    + serializedAddress
                                    + " will not be persisted for participant id: "
                                    + participantId);
                            } else {
                                persistency.setItem(
                                        that.getStorageKey(participantId),
                                        serializedAddress);
                            }

                            if (routingProxy !== undefined) {
                                // register remotely
                                promise = that.addNextHopToParentRoutingTable(participantId);
                            } else {
                                if (parentMessageRouterAddress !== undefined) {
                                    promise = new Promise(function(resolve, reject){
                                        queuedAddNextHopCalls[queuedAddNextHopCalls.length] =
                                        {
                                            participantId : participantId,
                                            resolve : resolve,
                                            reject : reject
                                        };
                                    });
                                } else {
                                    promise = Promise.resolve();
                                }
                            }
                            that.participantRegistered(participantId);
                            return promise;
                        };

                /**
                 * @function MessageRouter#participantRegistered
                 *
                 * @param {String} participantId
                 *
                 * @returns {Promise} a Promise object
                 */
                this.participantRegistered =
                        function participantRegistered(participantId) {
                            var i, msgContainer, messageQueue =
                                    settings.messageQueue.getAndRemoveMessages(participantId);

                            if (messageQueue !== undefined) {
                                i = messageQueue.length;
                                while (i--) {
                                    that.route(messageQueue[i]);
                                }
                            }
                        };

            }

            return MessageRouter;
        });
