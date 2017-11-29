/*jslint es5: true, node: true, node: true */
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
var Promise = require("../../../../classes/global/Promise");
var SharedMqttClient = require("../../../../classes/joynr/messaging/mqtt/SharedMqttClient");
var MqttAddress = require("../../../../classes/joynr/system/RoutingTypes/MqttAddress");
var MqttMessagingStub = require("../../../../classes/joynr/messaging/mqtt/MqttMessagingStub");
var JoynrMessage = require("../../../../classes/joynr/messaging/JoynrMessage");

describe("libjoynr-js.joynr.messaging.mqtt.MqttMessagingStub", function() {
    var destinationMqttAddress, topic;
    var mqttClient, mqttMessagingStub;
    var joynrMessage, multicastMessage;

    beforeEach(function(done) {
        topic = "testTopic";
        destinationMqttAddress = new MqttAddress({
            brokerUri: "testBrokerUri",
            topic: topic
        });
        mqttClient = jasmine.createSpyObj("mqttClient", ["send"]);
        mqttClient.send.and.returnValue(Promise.resolve());
        mqttMessagingStub = new MqttMessagingStub({
            address: destinationMqttAddress,
            client: mqttClient
        });

        joynrMessage = {
            key: "joynrMessage",
            to: "toParticipantId",
            type: "request"
        };
        multicastMessage = {
            key: "multicastMessage",
            to: "toParticipantId",
            type: JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST
        };

        done();
    });

    it("is instantiable and of correct type", function(done) {
        expect(MqttMessagingStub).toBeDefined();
        expect(typeof MqttMessagingStub).toEqual("function");
        expect(mqttMessagingStub).toBeDefined();
        expect(mqttMessagingStub instanceof MqttMessagingStub).toEqual(true);
        expect(mqttMessagingStub.transmit).toBeDefined();
        expect(typeof mqttMessagingStub.transmit).toEqual("function");
        done();
    });

    it("transmits a message", function(done) {
        var expectedTopic = topic + "/low/" + joynrMessage.to;
        mqttMessagingStub.transmit(joynrMessage);
        expect(mqttClient.send).toHaveBeenCalledWith(expectedTopic, joynrMessage);
        done();
    });

    it("keeps topic of multicast messages", function(done) {
        var expectedTopic = topic;
        mqttMessagingStub.transmit(multicastMessage);
        expect(mqttClient.send).toHaveBeenCalledWith(expectedTopic, multicastMessage);
        done();
    });
});
