/*jslint es5: true, nomen: true */

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

/**
 * The <code>MessageQueue</code> is a joynr internal data structure. The Message Queue caches incoming messages, which cannot be shipped
 * to the correct participant. Once a participant with the matching participantId is registered, the incoming message is forwarded to him
 */
define(
        "joynr/messaging/routing/MessageQueue",
        [
            "joynr/system/LoggerFactory",
            "joynr/system/DiagnosticTags",
            "joynr/util/LongTimer",
            "joynr/util/UtilInternal",
            "joynr/messaging/JoynrMessage"
        ],
        function(LoggerFactory, DiagnosticTags, LongTimer, Util, JoynrMessage) {

            var log = LoggerFactory.getLogger("joynr/messaging/routing/MessageQueue");
            var defaultSettings;
            /**
             * MessageQueue caches incoming messages, and cached messages can be retrieved in case the destination participant becomes visible
             *
             * @constructor
             * @name MessageQueue
             *
             * @param {Object}
             *            settings the settings object holding dependencies
             * @param {Number}
             *            settings.maxQueueSizeInKBytes the maximum buffer size that shall be allocated by the message queue
             *
             * @classdesc The <code>MessageQueue</code> is a joynr internal data structure. The Message Queue caches incoming messages, which cannot be shipped
             * to the correct participant. Once a participant with the matching participantId is registered, the incoming message is forwarded to him
             */
            function MessageQueue(settings) {

                this._messageQueues = {};
                Util.extend(this, defaultSettings, settings);
                this.currentQueueSize = 0;
            }

            MessageQueue.CHECK_TTL_ON_QUEUED_MESSAGES_INTERVAL_MS = 5000;
            MessageQueue.DEFAULT_MAX_QUEUE_SIZE_IN_KBYTES = 10000;

            defaultSettings = {
                maxQueueSizeInKBytes : MessageQueue.DEFAULT_MAX_QUEUE_SIZE_IN_KBYTES
            };

            /**
             * resets the queue
             *
             * @name MessageQueue#reset()
             * @function
             *
             */
            MessageQueue.prototype.reset = function reset() {
                this.currentQueueSize = 0;
                this._messageQueues = {};
            };

            MessageQueue.prototype._removeExpiredMessage = function(message) {
                var index, queue;
                queue = this._messageQueues[message.to];

                if (queue === undefined) {
                    return;
                }

                index = queue.indexOf(message);
                if (index !== -1) {
                    queue.splice(index, 1);
                }
            };

            MessageQueue.prototype._removeMessageWhenTtlExpired = function(message) {
                var ttl;
                ttl = message.expiryDate - Date.now();
                // some browsers do not support negative timeout times.
                ttl = ttl > 0 ? ttl : 0;

                LongTimer.setTimeout(this._removeExpiredMessage.bind(this, message), ttl);
            };

            /**
             * @name MessageQueue#putMessage
             * @function
             *
             * @param {JoynrMessage}
             *            joynrMessage
             */
            MessageQueue.prototype.putMessage =
                    function putMessage(message) {
                        // drop message if maximum queue size has been reached
                        if (message.payload !== undefined) {
                            var messageSize = Util.getLengthInBytes(message.payload);
                            if ((this.currentQueueSize + messageSize) <= (this.maxQueueSizeInKBytes * 1024)) {
                                this.currentQueueSize = this.currentQueueSize + messageSize;
                                if (this._messageQueues[message.to] === undefined) {
                                    this._messageQueues[message.to] = [];
                                }
                                this._messageQueues[message.to].push(message);
                                this._removeMessageWhenTtlExpired(message);
                            } else {
                                log
                                        .error(
                                                "message cannot be added to message queue, as the queue buffer size has been exceeded",
                                                DiagnosticTags.forJoynrMessage(message));
                            }
                        }
                    };

            /**
             * gets the queue messages for the participant
             *
             * @name MessageQueue#getAndRemoveMessages
             * @function
             *
             * @param {String}
             *            participantId
             */
            MessageQueue.prototype.getAndRemoveMessages =
                    function getAndRemoveMessages(participantId) {
                        var result = [];
                        if (this._messageQueues[participantId] !== undefined) {
                            result = this._messageQueues[participantId];
                            delete this._messageQueues[participantId];
                        }
                        return result;
                    };

            /**
             * Shutdown the message queue
             *
             * @function
             * @name MessageQueue#shutdown
             */
            MessageQueue.prototype.shutdown = function shutdown() {
                this._messageQueues = {};
                this.currentQueueSize = 0;
            };

            return MessageQueue;
        });
