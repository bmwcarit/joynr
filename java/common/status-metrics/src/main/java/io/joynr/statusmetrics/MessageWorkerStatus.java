/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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
package io.joynr.statusmetrics;

public class MessageWorkerStatus {
    /**
     * Whenever a message worker starts to process a message, this timestamp will be updated.
     * When the timespan between 'now' and the heartbeat timestamp becomes too large, an
     * error might have occurred which blocks the message worker's thread indefinitely.
     * Please note that the heartbeat also covers calls to user defined provider code.
     * Therefore a possible reason for an error could be that the provider code blocks
     * forever. The potential length of a provider operation must be taken into account
     * when evaluating this heartbeat value.
     *
     * Will not be updated, when the message worker is waiting for a new message.
     * @see isWaitingForMessage
     * @return Returns the heartbeat timestamp in unix time (ms since 01.01.1970)
     */
    private long heartbeatTimestamp;

    /**
     * Will be set to true, if the message queue is empty and the message worker is waiting
     * for a message to arrive. During this phase heartbeatTimestamp will not be updated.
     */
    private boolean isWaitingForMessage;

    public MessageWorkerStatus(long heartbeatTimestamp, boolean isWaitingForMessage) {
        this.heartbeatTimestamp = heartbeatTimestamp;
        this.isWaitingForMessage = isWaitingForMessage;
    }

    public long getHeartbeatTimestamp() {
        return heartbeatTimestamp;
    }

    public boolean isWaitingForMessage() {
        return isWaitingForMessage;
    }
}