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

import ParticipantQueue from "../../../../../main/js/joynr/messaging/routing/ParticipantQueue";
import JoynrMessage from "../../../../../main/js/joynr/messaging/JoynrMessage";

let fakeTime: number;

function increaseFakeTime(timeMs: number) {
    fakeTime = fakeTime + timeMs;
    jest.advanceTimersByTime(timeMs);
}

describe("libjoynr-js.joynr.messaging.routing.ParticipantQueue", () => {
    let participantQueue: ParticipantQueue;
    const receiverParticipantId = `TestparticipantQueue_participantId_${Date.now()}`;
    const joynrMessage = new JoynrMessage({
        type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
        payload: "hello"
    });
    joynrMessage.to = receiverParticipantId;
    joynrMessage.from = "senderParticipantId";
    const joynrMessage2 = new JoynrMessage({
        type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
        payload: "hello2"
    });
    joynrMessage2.to = receiverParticipantId;
    joynrMessage2.from = "senderParticipantId2";

    beforeEach(done => {
        participantQueue = new ParticipantQueue();
        fakeTime = Date.now();
        jest.useFakeTimers();
        jest.spyOn(Date, "now").mockImplementation(() => {
            return fakeTime;
        });
        done();
    });

    afterEach(done => {
        jest.useRealTimers();
        done();
    });

    it("increases queueSize when calling putMessage", () => {
        const queueSize = participantQueue.getSize();
        participantQueue.putMessage(joynrMessage, joynrMessage.payload.length);
        expect(participantQueue.getSize()).toBe(queueSize + joynrMessage.payload.length);
    });

    it("returns inserted messages when calling getMessages", () => {
        participantQueue.putMessage(joynrMessage, joynrMessage.payload.length);
        participantQueue.putMessage(joynrMessage2, joynrMessage2.payload.length);
        const queue = participantQueue.getMessages();
        expect(queue).toEqual([joynrMessage, joynrMessage2]);
    });

    it("filters expired messages", () => {
        joynrMessage.expiryDate = Date.now() + 1000;
        joynrMessage2.expiryDate = Date.now() + 2000;
        participantQueue.putMessage(joynrMessage, joynrMessage.payload.length);
        participantQueue.putMessage(joynrMessage2, joynrMessage2.payload.length);
        increaseFakeTime(1001);
        participantQueue.filterExpiredMessages();
        expect(participantQueue.getMessages()).toEqual([joynrMessage2]);
    });

    it("filters expired messages with first ttl > second ttl", () => {
        joynrMessage.expiryDate = Date.now() + 2000;
        joynrMessage2.expiryDate = Date.now() + 1000;
        participantQueue.putMessage(joynrMessage, joynrMessage.payload.length);
        participantQueue.putMessage(joynrMessage2, joynrMessage2.payload.length);
        increaseFakeTime(1001);
        participantQueue.filterExpiredMessages();
        expect(participantQueue.getMessages()).toEqual([joynrMessage]);
    });
});
