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
import * as UtilInternal from "../../util/UtilInternal";
import JoynrMessage = require("../JoynrMessage");

class ParticipantQueue {
    public currentQueueSize = 0;
    private queue: JoynrMessage[];
    /**
     * This is a helper class for MessageQueue. It manages the messages waiting for a single participant.
     * It keeps its queue size which can be used by the messageQueue to calculate the total queue size of all participant
     * queues.
     * It allows queued messages to be filtered by expiry date.
     *
     * @constructor
     */
    public constructor() {
        this.queue = [];
    }

    /**
     * filters expired messages
     *
     */
    public filterExpiredMessages(): void {
        const now = Date.now();
        let totalBytesRemoved = 0;

        const newQueue = [];
        let i;
        for (i = 0; i < this.queue.length; i++) {
            const msg = this.queue[i];
            if (now > msg.expiryDate) {
                totalBytesRemoved += UtilInternal.getLengthInBytes(msg.payload);
            } else {
                newQueue.push(msg);
            }
        }
        this.currentQueueSize -= totalBytesRemoved;
        this.queue = newQueue;
    }

    /**
     * puts messages in queue
     *
     */
    public putMessage(message: JoynrMessage, size: number): void {
        this.currentQueueSize += size;
        this.queue.push(message);
    }

    public getMessages(): JoynrMessage[] {
        return this.queue;
    }

    public getSize(): number {
        return this.currentQueueSize;
    }
}

export = ParticipantQueue;
