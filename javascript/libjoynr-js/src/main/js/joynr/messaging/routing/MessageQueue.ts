/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
import LoggingManager from "../../system/LoggingManager";

import * as DiagnosticTags from "../../system/DiagnosticTags";
import * as UtilInternal from "../../util/UtilInternal";
import ParticipantQueue from "./ParticipantQueue";
import JoynrMessage = require("../JoynrMessage");

const log = LoggingManager.getLogger("joynr/messaging/routing/MessageQueue");
const CHECK_TTL_ON_QUEUED_MESSAGES_INTERVAL_MS = 10000; // a very loose interval because of a second check on return

class MessageQueue {
    public static DEFAULT_MAX_QUEUE_SIZE_IN_KBYTES = 10000;
    public cleanupInterval: NodeJS.Timer;
    public currentQueueSize = 0;
    private participantQueues: Record<string, any> = {};
    public maxQueueSizeInKBytes = MessageQueue.DEFAULT_MAX_QUEUE_SIZE_IN_KBYTES;
    /**
     * MessageQueue caches incoming messages, and cached messages can be retrieved in case the destination participant becomes visible
     *
     * @constructor
     *
     * @param settings the settings object holding dependencies
     * @param settings.maxQueueSizeInKBytes the maximum buffer size that shall be allocated by the message queue
     *
     * @classdesc The <code>MessageQueue</code> is a joynr internal data structure. The Message Queue caches incoming messages, which cannot be shipped
     * to the correct participant. Once a participant with the matching participantId is registered, the incoming message is forwarded to him
     */
    public constructor(settings: { maxQueueSizeInKBytes?: number }) {
        if (settings.maxQueueSizeInKBytes !== undefined) this.maxQueueSizeInKBytes = settings.maxQueueSizeInKBytes;
        this.cleanupInterval = setInterval(() => {
            // TODO: we could call this way more lazy -> make an if and only call this if this.currentQueueSize > 100
            let newSize = 0;
            for (const id in this.participantQueues) {
                if (Object.prototype.hasOwnProperty.call(this.participantQueues, id)) {
                    this.participantQueues[id].filterExpiredMessages();
                    const size = this.participantQueues[id].getSize();
                    newSize += size;
                    if (size === 0) {
                        delete this.participantQueues[id];
                    }
                }
            }
            this.currentQueueSize = newSize;
        }, CHECK_TTL_ON_QUEUED_MESSAGES_INTERVAL_MS);
    }

    public putMessage(message: JoynrMessage): void {
        // drop message if maximum queue size has been reached
        if (message.payload !== undefined) {
            const messageSize = UtilInternal.getLengthInBytes(message.payload);
            if (this.currentQueueSize + messageSize <= this.maxQueueSizeInKBytes * 1024) {
                this.currentQueueSize = this.currentQueueSize + messageSize;
                if (this.participantQueues[message.to] === undefined) {
                    this.participantQueues[message.to] = new ParticipantQueue();
                }
                this.participantQueues[message.to].putMessage(message, messageSize);
            } else {
                log.error(
                    "message cannot be added to message queue, as the queue buffer size has been exceeded",
                    DiagnosticTags.forJoynrMessage(message)
                );
            }
        }
    }

    /**
     * gets the queue messages for the participant
     *
     * @param participantId
     */
    public getAndRemoveMessages(participantId: string): JoynrMessage[] {
        let result = [];
        if (this.participantQueues[participantId] !== undefined) {
            const participantQueue = this.participantQueues[participantId];
            this.currentQueueSize -= participantQueue.getSize();
            participantQueue.filterExpiredMessages();
            result = participantQueue.getMessages();
            delete this.participantQueues[participantId];
        }
        return result;
    }

    /**
     * Shutdown the message queue
     */
    public shutdown(): void {
        this.participantQueues = {};
        this.currentQueueSize = 0;
        clearInterval(this.cleanupInterval);
    }
}

export = MessageQueue;
