/*jslint es5: true, node: true, node: true */
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
var Promise = require('../../../global/Promise');
var Mqtt = require('../../../global/Mqtt');
var JoynrMessage = require('../JoynrMessage');
var MessagingQosEffort = require('../MessagingQosEffort');
var JsonSerializer = require('../../util/JSONSerializer');
var LongTimer = require('../../util/LongTimer');
var Typing = require('../../util/Typing');
var LoggerFactory = require('../../system/LoggerFactory');
module.exports = (function (Promise, Mqtt, JoynrMessage, MessagingQosEffort, JSONSerializer, LongTimer, Typing, LoggerFactory) {
            var log = LoggerFactory.getLogger("joynr.messaging.mqtt.SharedMqttClient");

            /**
             * @param {mqtt}
             *            client to use for sending messages
             * @param {Array}
             *            queuedMessages
             */
            function sendQueuedMessages(client, queuedMessages) {
                var queued, topic;
                while (queuedMessages.length) {
                    queued = queuedMessages.shift();
                    try {
                        client.publish(queued.topic, JSONSerializer.stringify(queued.message), queued.options);
                        queued.resolve();
                        // Error is thrown if the connection is no longer open
                    } catch (e) {
                        // so add the message back to the front of the queue
                        queuedMessages.unshift(queued);
                        throw e;
                    }
                }
            }

            function sendQueuedUnsubscriptions(client, queuedUnsubscriptions) {
                var i, topic;
                for (i = 0; i < queuedUnsubscriptions.length; i++) {
                    client.unsubscribe(queuedUnsubscriptions[i]);
                }
                queuedUnsubscriptions = [];
            }

            function sendQueuedSubscriptions(client, queuedSubscriptions, qosLevel) {
                return new Promise(function(resolve, reject) {
                    var i, topic, subscribeObject = {};
                    for (i = 0; i < queuedSubscriptions.length; i++) {
                        topic = queuedSubscriptions[i];
                        subscribeObject[topic] = qosLevel;
                    }
                    client.subscribe(subscribeObject, undefined, function(err, granted) {
                        //TODO error handling
                        queuedSubscriptions = [];
                        resolve();
                    });
                });
            }

            function sendMessage(client, topic, joynrMessage, sendQosLevel, queuedMessages) {
                return new Promise(function(resolve, reject){
                    try {
                        client.publish(topic, JSONSerializer.stringify(joynrMessage), { qos : sendQosLevel });
                        resolve();
                        // Error is thrown if the socket is no longer open, so requeue to the front
                    } catch (e) {
                        // add the message back to the front of the queue
                        queuedMessages.unshift({
                            message : joynrMessage,
                            resolve : resolve,
                            options : {
                                qos : sendQosLevel,
                            },
                            topic : topic
                        });
                        throw e;
                    }
                });
            }

            /**
             * @name SharedMqttClient
             * @constructor
             * @param {Object}
             *            settings
             * @param {MqttAddress}
             *            settings.address to be used to connect to mqtt broker
             */
            var SharedMqttClient =
                    function SharedMqttClient(settings) {
                        Typing.checkProperty(settings, "Object", "settings");
                        Typing.checkProperty(
                                settings.address,
                                "MqttAddress",
                                "settings.address");

                        var client = null;
                        var address = settings.address;
                        var onmessageCallback = null;
                        var queuedMessages = [];
                        var onOpen;
                        var resolve;
                        var closed = false;
                        var connected = false;
                        var onConnectedPromise = new Promise(function(resolveTrigger) {
                            resolve = resolveTrigger;
                        });
                        var qosLevel = SharedMqttClient.DEFAULT_QOS_LEVEL;

                        if (settings.provisioning.qosLevel !== undefined) {
                            qosLevel = settings.provisioning.qosLevel;
                        }

                        var queuedSubscriptions = [];
                        var queuedUnsubscriptions = [];

                        var onMessage = function(topic, payload) {
                            if (onmessageCallback !== undefined) {
                                onmessageCallback(topic, new JoynrMessage(JSON.parse(payload.toString())));
                            }
                        };

                        var resetConnection = function resetConnection() {
                            if (closed) {
                                return;
                            }
                            client = new Mqtt.connect(address.brokerUri);
                            client.on('connect', onOpen);
                            client.on('message', onMessage);
                        };

                        this.onConnected = function() {
                            return onConnectedPromise;
                        };

                        // send all queued messages, requeuing to the front in case of a problem
                        onOpen = function onOpen() {
                            try {
                                connected = true;
                                sendQueuedMessages(client, queuedMessages);
                                sendQueuedUnsubscriptions(client, queuedUnsubscriptions);
                                sendQueuedSubscriptions(client, queuedSubscriptions, qosLevel).then(resolve);
                            } catch (e) {
                                resetConnection();
                            }
                        };

                        resetConnection();

                        /**
                         * @name SharedMqttClient#send
                         * @function
                         * @param {String}
                         *            topic the topic to publish the message
                         * @param {JoynrMessage}
                         *            joynrMessage the joynr message to transmit
                         */
                        this.send = function send(topic, joynrMessage) {
                            var sendQosLevel = qosLevel;
                            if (MessagingQosEffort.BEST_EFFORT === joynrMessage.effort) {
                                sendQosLevel = SharedMqttClient.BEST_EFFORT_QOS_LEVEL;
                            }

                            return sendMessage(client, topic, joynrMessage, sendQosLevel, queuedMessages).catch(function(e1) {
                                    resetConnection();
                                });
                        };

                        /**
                         * @name SharedMqttClient#close
                         * @function
                         */
                        this.close = function close() {
                            closed = true;
                            if (client !== null && client.end) {
                                client.end();
                            }
                        };

                        this.subscribe = function(topic) {
                            if (connected) {
                                client.subscribe(topic, { qos : qosLevel} );
                            } else {
                                queuedSubscriptions.push(topic);
                            }
                        };

                        this.unsubscribe = function(topic) {
                            if (connected) {
                                client.unsubscribe(topic);
                            } else {
                                queuedUnsubscriptions.push(topic);
                            }
                        };

                        // using the defineProperty syntax for onmessage to be able to keep
                        // the same API as WebSocket but have a setter function called when
                        // the attribute is set.
                        Object.defineProperty(this, "onmessage", {
                            set : function(newCallback) {
                                onmessageCallback = newCallback;
                                if (typeof newCallback !== "function") {
                                    throw new Error(
                                            "onmessage callback must be a function, but instead was of type "
                                                + typeof newCallback);
                                }
                            },
                            get : function() {
                                return onmessageCallback;
                            },
                            enumerable : false,
                            configurable : false
                        });

                    };

            SharedMqttClient.DEFAULT_QOS_LEVEL = 1;
            SharedMqttClient.BEST_EFFORT_QOS_LEVEL = 0;

            return SharedMqttClient;
}(Promise, Mqtt, JoynrMessage, MessagingQosEffort, JsonSerializer, LongTimer, Typing, LoggerFactory));