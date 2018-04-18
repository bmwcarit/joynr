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
const LoggingManager = require("../../system/LoggingManager");
const DiagnosticTags = require("../../system/DiagnosticTags");
const UtilInternal = require("../../util/UtilInternal");
const ParticipantQueue = require("./ParticipantQueue");

const log = LoggingManager.getLogger("joynr/messaging/routing/MessageQueue");
let defaultSettings;
const CHECK_TTL_ON_QUEUED_MESSAGES_INTERVAL_MS = 10000; // a very loose interval because of a second check on return
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
    UtilInternal.extend(this, defaultSettings, settings);
    this.currentQueueSize = 0;

    const that = this;
    this.cleanupInterval = setInterval(() => {
        // TODO: we could call this way more lazy -> make an if and only call this if this.currentQueueSize > 100
        let newSize = 0;
        let id;
        for (id in that._participantQueues) {
            if (that._participantQueues.hasOwnProperty(id)) {
                that._participantQueues[id].filterExpiredMessages();
                const size = that._participantQueues[id].getSize();
                newSize += size;
                if (size === 0) {
                    delete that._participantQueues[id];
                }
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
 * @name MessageQueue#putMessage
 * @function
 *
 * @param {JoynrMessage}
 *            joynrMessage
 */
MessageQueue.prototype.putMessage = function putMessage(message) {
    // drop message if maximum queue size has been reached
    if (message.payload !== undefined) {
        const messageSize = UtilInternal.getLengthInBytes(message.payload);
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
    let result = [];
    if (this._participantQueues[participantId] !== undefined) {
        const participantQueue = this._participantQueues[participantId];
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
