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

import MqttAddress from "../../../../../main/js/generated/joynr/system/RoutingTypes/MqttAddress";
import MqttMessagingStub from "../../../../../main/js/joynr/messaging/mqtt/MqttMessagingStub";
import JoynrMessage from "../../../../../main/js/joynr/messaging/JoynrMessage";

describe("libjoynr-js.joynr.messaging.mqtt.MqttMessagingStub", () => {
    let destinationMqttAddress: any, topic: any;
    let mqttClient: any, mqttMessagingStub: MqttMessagingStub;
    let joynrMessage: JoynrMessage, multicastMessage: any;

    beforeEach(done => {
        topic = "testTopic";
        destinationMqttAddress = new MqttAddress({
            brokerUri: "testBrokerUri",
            topic
        });
        mqttClient = {
            send: jest.fn()
        };
        mqttClient.send.mockReturnValue(Promise.resolve());
        mqttMessagingStub = new MqttMessagingStub({
            address: destinationMqttAddress,
            client: mqttClient
        });

        joynrMessage = new JoynrMessage({
            payload: "joynrMessage",
            type: "request"
        });
        joynrMessage.to = "toParticipantId";

        multicastMessage = new JoynrMessage({
            payload: "multicastMessage",
            type: JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST
        });
        multicastMessage.to = "toParticipantId";

        done();
    });

    it("is instantiable and of correct type", () => {
        expect(MqttMessagingStub).toBeDefined();
        expect(typeof MqttMessagingStub).toEqual("function");
        expect(mqttMessagingStub).toBeDefined();
        expect(mqttMessagingStub.transmit).toBeDefined();
        expect(typeof mqttMessagingStub.transmit).toEqual("function");
    });

    it("transmits a message", () => {
        const expectedTopic = `${topic}/low`;
        mqttMessagingStub.transmit(joynrMessage);
        expect(mqttClient.send).toHaveBeenCalledWith(expectedTopic, joynrMessage);
    });

    it("keeps topic of multicast messages", () => {
        const expectedTopic = topic;
        mqttMessagingStub.transmit(multicastMessage);
        expect(mqttClient.send).toHaveBeenCalledWith(expectedTopic, multicastMessage);
    });
});
