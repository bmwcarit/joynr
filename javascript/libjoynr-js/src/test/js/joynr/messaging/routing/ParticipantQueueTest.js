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
require("../../../node-unit-test-helper");
const ParticipantQueue = require("../../../../../main/js/joynr/messaging/routing/ParticipantQueue");
const JoynrMessage = require("../../../../../main/js/joynr/messaging/JoynrMessage");
const Date = require("../../../../../test/js/global/Date");

let fakeTime;

function increaseFakeTime(time_ms) {
    fakeTime = fakeTime + time_ms;
    jasmine.clock().tick(time_ms);
}

describe("libjoynr-js.joynr.messaging.routing.ParticipantQueue", () => {
    let participantQueue, joynrMessage, joynrMessage2, receiverParticipantId;
    receiverParticipantId = "TestparticipantQueue_participantId_" + Date.now();
    joynrMessage = new JoynrMessage({
        type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
        payload: "hello"
    });
    joynrMessage.to = receiverParticipantId;
    joynrMessage.from = "senderParticipantId";
    joynrMessage2 = new JoynrMessage({
        type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
        payload: "hello2"
    });
    joynrMessage2.to = receiverParticipantId;
    joynrMessage2.from = "senderParticipantId2";

    beforeEach(done => {
        participantQueue = new ParticipantQueue({});
        fakeTime = Date.now();
        jasmine.clock().install();
        spyOn(Date, "now").and.callFake(() => {
            return fakeTime;
        });
        done();
    });

    afterEach(done => {
        jasmine.clock().uninstall();
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
