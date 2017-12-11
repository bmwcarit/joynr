/*jslint es5: true, nomen: true, node: true */

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
var LoggerFactory = require("../../system/LoggerFactory");
var DiagnosticTags = require("../../system/DiagnosticTags");
var Util = require("../../util/UtilInternal");
var ParticipantQueue = require("./ParticipantQueue");

var log = LoggerFactory.getLogger("joynr/messaging/routing/MessageQueue");
var defaultSettings;
var CHECK_TTL_ON_QUEUED_MESSAGES_INTERVAL_MS = 10000; // a very loose interval because of a second check on return
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
    this._participantQueues = {};
    Util.extend(this, defaultSettings, settings);
    this.currentQueueSize = 0;

    var that = this;
    this.cleanupInterval = setInterval(function() {
        // TODO: we could call this way more lazy -> make an if and only call this if this.currentQueueSize > 100
        var newSize = 0;
        var id;
        for (id in that._participantQueues) {
            if (that._participantQueues.hasOwnProperty(id)) {
                that._participantQueues[id].filterExpiredMessages();
                newSize += that._participantQueues[id].getSize();
            }
        }
        that.currentQueueSize = newSize;
    }, CHECK_TTL_ON_QUEUED_MESSAGES_INTERVAL_MS);
}

MessageQueue.DEFAULT_MAX_QUEUE_SIZE_IN_KBYTES = 10000;

defaultSettings = {
    maxQueueSizeInKBytes: MessageQueue.DEFAULT_MAX_QUEUE_SIZE_IN_KBYTES
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
    this._participantQueues = {};
};

/**
 * @name MessageQueue#putMessage
 * @function
 *
 * @param {JoynrMessage}
 *            joynrMessage
 */
MessageQueue.prototype.putMessage = function putMessage(message) {
    // drop message if maximum queue size has been reached
    if (message.payload !== undefined) {
        var messageSize = Util.getLengthInBytes(message.payload);
        if (this.currentQueueSize + messageSize <= this.maxQueueSizeInKBytes * 1024) {
            this.currentQueueSize = this.currentQueueSize + messageSize;
            if (this._participantQueues[message.to] === undefined) {
                this._participantQueues[message.to] = new ParticipantQueue();
            }
            this._participantQueues[message.to].putMessage(message, messageSize);
        } else {
            log.error(
                "message cannot be added to message queue, as the queue buffer size has been exceeded",
                DiagnosticTags.forJoynrMessage(message)
            );
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
MessageQueue.prototype.getAndRemoveMessages = function getAndRemoveMessages(participantId) {
    var result = [];
    if (this._participantQueues[participantId] !== undefined) {
        var participantQueue = this._participantQueues[participantId];
        this.currentQueueSize -= participantQueue.getSize();
        participantQueue.filterExpiredMessages();
        result = participantQueue.getMessages();
        delete this._participantQueues[participantId];
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
    this._participantQueues = {};
    this.currentQueueSize = 0;
    clearInterval(this.cleanupInterval);
};

module.exports = MessageQueue;
