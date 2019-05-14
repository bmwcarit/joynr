/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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

const UtilInternal = require("../../util/UtilInternal");

/**
 * This is a helper class for MessageQueue. It manages the messages waiting for a single participant.
 * It keeps its queue size which can be used by the messageQueue to calculate the total queue size of all participant
 * queues.
 * It allows queued messages to be filtered by expiry date.
 *
 * @constructor
 * @name ParticipantQueue
 */
function ParticipantQueue() {
    this._queue = [];
    this.currentQueueSize = 0;
}

/**
 * filters expired messages
 *
 * @name ParticipantQueue#filterExpiredMessages
 * @function
 *
 */
ParticipantQueue.prototype.filterExpiredMessages = function() {
    const now = Date.now();
    let totalBytesRemoved = 0;

    const newQueue = [];
    let i;
    for (i = 0; i < this._queue.length; i++) {
        const msg = this._queue[i];
        if (now > msg.expiryDate) {
            totalBytesRemoved += UtilInternal.getLengthInBytes(msg.payload);
        } else {
            newQueue.push(msg);
        }
    }
    this.currentQueueSize -= totalBytesRemoved;
    this._queue = newQueue;
};

/**
 * puts messages in queue
 *
 * @name ParticipantQueue#putMessage
 * @function
 *
 */
ParticipantQueue.prototype.putMessage = function putMessage(message, size) {
    this.currentQueueSize += size;
    this._queue.push(message);
};

ParticipantQueue.prototype.getMessages = function() {
    return this._queue;
};

ParticipantQueue.prototype.getSize = function() {
    return this.currentQueueSize;
};

module.exports = ParticipantQueue;
