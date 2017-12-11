/*jslint node: true */

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
var Util = require("../../../../classes/joynr/util/UtilInternal");
var MessageQueue = require("../../../../classes/joynr/messaging/routing/MessageQueue");
var JoynrMessage = require("../../../../classes/joynr/messaging/JoynrMessage");
var Date = require("../../../../test-classes/global/Date");

var fakeTime;

function increaseFakeTime(time_ms) {
    fakeTime = fakeTime + time_ms;
    jasmine.clock().tick(time_ms);
}

describe("libjoynr-js.joynr.messaging.routing.MessageQueue", function() {
    var messageQueue, joynrMessage, joynrMessage2, receiverParticipantId;
    receiverParticipantId = "TestMessageQueue_participantId_" + Date.now();
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

    messageQueue = new MessageQueue({
        maxQueueSizeInKBytes: 0.5
        // set the qsize to 500 bytes for testing purposes
    });

    beforeEach(function(done) {
        fakeTime = Date.now();
        jasmine.clock().install();
        spyOn(Date, "now").and.callFake(function() {
            return fakeTime;
        });
        messageQueue.reset();
        done();
    });

    afterEach(function(done) {
        jasmine.clock().uninstall();
        done();
    });

    it("test message queue limit", function(done) {
        var newJoynrMessage,
            i = 0,
            payload = "hello",
            oldQueueSize,
            maxIterations = Math.floor(messageQueue.maxQueueSizeInKBytes * 1024 / Util.getLengthInBytes(payload));
        newJoynrMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
            payload: payload
        });
        newJoynrMessage.expiryDate = Date.now() + 1000;

        while (i < maxIterations) {
            newJoynrMessage.to = receiverParticipantId + "i";
            newJoynrMessage.from = "senderParticipantId" + "i";
            oldQueueSize = messageQueue.currentQueueSize;
            messageQueue.putMessage(joynrMessage);
            i++;
            //until now, all messages shall be in the queue
            expect(messageQueue.currentQueueSize).toEqual(oldQueueSize + Util.getLengthInBytes(payload));
        }
        //now, the next message shall lead to a queue overflow
        newJoynrMessage.to = receiverParticipantId + "ExceedsQueueBuffer";
        newJoynrMessage.from = "senderParticipantId" + "ExceedsQueueBuffer";
        oldQueueSize = messageQueue.currentQueueSize;
        i = 0;
        while (i < 10) {
            messageQueue.putMessage(joynrMessage);
            expect(messageQueue.currentQueueSize).toEqual(oldQueueSize);
            i++;
        }
        done();
    });

    it("decreases currentQueueSize upon removing a participantQueue", function(done) {
        var payload = new Array(10).join("a");

        var joynrMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
            payload: payload
        });
        joynrMessage.to = receiverParticipantId;
        joynrMessage.from = "senderParticipantId";
        var initialQueueSize = messageQueue.currentQueueSize;
        messageQueue.putMessage(joynrMessage);
        expect(messageQueue.currentQueueSize).toBe(initialQueueSize + payload.length);

        messageQueue.getAndRemoveMessages(receiverParticipantId);
        expect(messageQueue.currentQueueSize).toEqual(initialQueueSize);

        done();
    });

    it("put Message adds new queued message, dropped after getAndRemoveMessage call", function(done) {
        var queuedMessages;
        joynrMessage.expiryDate = Date.now() + 1000;
        messageQueue.putMessage(joynrMessage);

        queuedMessages = messageQueue.getAndRemoveMessages(receiverParticipantId);

        expect(queuedMessages.length).toEqual(1);
        expect(queuedMessages[0]).toEqual(joynrMessage);

        expect(messageQueue.getAndRemoveMessages(receiverParticipantId).length).toEqual(0);
        done();
    });

    it("put Message adds multiple queued messages, dropped after getAndRemoveMessage call", function(done) {
        var queuedMessages;
        joynrMessage.expiryDate = Date.now() + 1000;
        joynrMessage2.expiryDate = Date.now() + 1000;
        messageQueue.putMessage(joynrMessage);
        messageQueue.putMessage(joynrMessage2);

        queuedMessages = messageQueue.getAndRemoveMessages(receiverParticipantId);

        expect(queuedMessages.length).toEqual(2);
        expect(queuedMessages[0]).toEqual(joynrMessage);
        expect(queuedMessages[1]).toEqual(joynrMessage2);

        expect(messageQueue.getAndRemoveMessages(receiverParticipantId).length).toEqual(0);
        done();
    });

    it("put Message adds new queued message, dropped after timeout", function(done) {
        var queuedMessages;
        joynrMessage.expiryDate = Date.now() + 1000;
        var initialQueueSize = messageQueue.currentQueueSize;
        messageQueue.putMessage(joynrMessage);
        expect(messageQueue.currentQueueSize).toBeGreaterThan(initialQueueSize);

        increaseFakeTime(1000 + 1);

        queuedMessages = messageQueue.getAndRemoveMessages(receiverParticipantId);
        expect(messageQueue.currentQueueSize).toEqual(initialQueueSize);
        expect(queuedMessages.length).toEqual(0);
        done();
    });

    it("put Message adds multiple queued messages, dropped first one after timeout", function(done) {
        var queuedMessages;
        joynrMessage.expiryDate = Date.now() + 1000;
        joynrMessage2.expiryDate = Date.now() + 2000;
        messageQueue.putMessage(joynrMessage);
        messageQueue.putMessage(joynrMessage2);

        increaseFakeTime(1000 + 1);

        queuedMessages = messageQueue.getAndRemoveMessages(receiverParticipantId);

        expect(queuedMessages.length).toEqual(1);
        expect(queuedMessages[0]).toEqual(joynrMessage2);
        expect(messageQueue.getAndRemoveMessages(receiverParticipantId).length).toEqual(0);
        done();
    });
    it(" empty message queue when shut down", function() {
        expect(messageQueue.currentQueueSize).toEqual(0);
        messageQueue.putMessage(joynrMessage);
        expect(messageQueue.currentQueueSize > 0).toBeTruthy();

        messageQueue.shutdown();
        expect(messageQueue.currentQueueSize).toEqual(0);
    });
});
