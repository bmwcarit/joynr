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
const Promise = require("../../../../../main/js/global/Promise");
const SharedMqttClient = require("../../../../../main/js/joynr/messaging/mqtt/SharedMqttClient");
const MqttAddress = require("../../../../../main/js/generated/joynr/system/RoutingTypes/MqttAddress");
const MqttMessagingStub = require("../../../../../main/js/joynr/messaging/mqtt/MqttMessagingStub");
const JoynrMessage = require("../../../../../main/js/joynr/messaging/JoynrMessage");

describe("libjoynr-js.joynr.messaging.mqtt.MqttMessagingStub", () => {
    let destinationMqttAddress, topic;
    let mqttClient, mqttMessagingStub;
    let joynrMessage, multicastMessage;

    beforeEach(done => {
        topic = "testTopic";
        destinationMqttAddress = new MqttAddress({
            brokerUri: "testBrokerUri",
            topic
        });
        mqttClient = jasmine.createSpyObj("mqttClient", ["send"]);
        mqttClient.send.and.returnValue(Promise.resolve());
        mqttMessagingStub = new MqttMessagingStub({
            address: destinationMqttAddress,
            client: mqttClient
        });

        joynrMessage = new JoynrMessage({
            key: "joynrMessage",
            type: "request"
        });
        joynrMessage.to = "toParticipantId";

        multicastMessage = new JoynrMessage({
            key: "multicastMessage",
            type: JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST
        });
        multicastMessage.to = "toParticipantId";

        done();
    });

    it("is instantiable and of correct type", done => {
        expect(MqttMessagingStub).toBeDefined();
        expect(typeof MqttMessagingStub).toEqual("function");
        expect(mqttMessagingStub).toBeDefined();
        expect(mqttMessagingStub instanceof MqttMessagingStub).toEqual(true);
        expect(mqttMessagingStub.transmit).toBeDefined();
        expect(typeof mqttMessagingStub.transmit).toEqual("function");
        done();
    });

    it("transmits a message", done => {
        const expectedTopic = topic + "/low/" + joynrMessage.to;
        mqttMessagingStub.transmit(joynrMessage);
        expect(mqttClient.send).toHaveBeenCalledWith(expectedTopic, joynrMessage);
        done();
    });

    it("keeps topic of multicast messages", done => {
        const expectedTopic = topic;
        mqttMessagingStub.transmit(multicastMessage);
        expect(mqttClient.send).toHaveBeenCalledWith(expectedTopic, multicastMessage);
        done();
    });
});
