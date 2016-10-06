/*jslint es5: true */

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
        "joynr/messaging/mqtt/SharedMqttClient",
        [
            "global/Promise",
            "global/Mqtt",
            "joynr/messaging/JoynrMessage",
            "joynr/util/Util",
            "joynr/util/JSONSerializer",
            "joynr/util/LongTimer",
            "joynr/system/LoggerFactory"
        ],
        function(Promise, Mqtt, JoynrMessage, Util, JSONSerializer, LongTimer, LoggerFactory) {
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

            function sendMessage(client, topic, joynrMessage, qosLevel, queuedMessages) {
                return new Promise(function(resolve, reject){
                    try {
                        client.publish(topic, JSONSerializer.stringify(joynrMessage), { qos : qosLevel });
                        resolve();
                        // Error is thrown if the socket is no longer open, so requeue to the front
                    } catch (e) {
                        // add the message back to the front of the queue
                        queuedMessages.unshift({
                            message : joynrMessage,
                            resolve : resolve,
                            options : {
                                qos : qosLevel,
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
                        Util.checkProperty(settings, "Object", "settings");
                        Util.checkProperty(
                                settings.address,
                                "MqttAddress",
                                "settings.address");

                        var client = null;
                        var address = settings.address;
                        var onmessageCallback = null;
                        var queuedMessages = [];
                        var onOpen;
                        var closed = false;

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

                        // send all queued messages, requeuing to the front in case of a problem
                        onOpen = function onOpen() {
                            try {
                                sendQueuedMessages(client, queuedMessages);
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
                         * @param {Number}
                         *            qosLevel qos level for message
                         */
                        this.send = function send(topic, joynrMessage, qosLevel) {
                            try {
                                Util.checkProperty(joynrMessage, "JoynrMessage", "joynrMessage");
                            } catch (e) {
                                throw e;
                            }
                            return sendMessage(client, topic, joynrMessage, qosLevel, queuedMessages).catch(function(e1) {
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
                            client.subscribe(topic);
                        };

                        this.unsubscribe = function(topic) {
                            client.unsubscribe(topic);
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

            return SharedMqttClient;
        });
