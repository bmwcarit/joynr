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
const SharedMqttClient = require("../../../../../main/js/joynr/messaging/mqtt/SharedMqttClient");
const MqttAddress = require("../../../../../main/js/generated/joynr/system/RoutingTypes/MqttAddress");
const MqttMessagingStubFactory = require("../../../../../main/js/joynr/messaging/mqtt/MqttMessagingStubFactory");
const JoynrMessage = require("../../../../../main/js/joynr/messaging/JoynrMessage");

describe("libjoynr-js.joynr.messaging.mqtt.MqttMessagingStubFactory", () => {
    let mqttMessagingStubFactory, mqttClient;
    let mqttAddress, brokerUri, topic, joynrMessage;

    beforeEach(done => {
        mqttClient = Object.create(SharedMqttClient.prototype);
        mqttClient.send = jasmine.createSpy("channelMessagingSender.send");
        mqttClient.send.and.returnValue(Promise.resolve());

        mqttMessagingStubFactory = new MqttMessagingStubFactory({
            client: mqttClient
        });

        brokerUri = "testBrokerUri";
        topic = "testTopic";
        mqttAddress = new MqttAddress({
            brokerUri,
            topic
        });
        joynrMessage = new JoynrMessage({
            key: "joynrMessage" // TODO understand what is this key thing?
        });

        done();
    });

    it("is instantiable and of correct type", done => {
        expect(MqttMessagingStubFactory).toBeDefined();
        expect(typeof MqttMessagingStubFactory).toEqual("function");
        expect(mqttMessagingStubFactory).toBeDefined();
        expect(mqttMessagingStubFactory instanceof MqttMessagingStubFactory).toEqual(true);
        expect(mqttMessagingStubFactory.build).toBeDefined();
        expect(typeof mqttMessagingStubFactory.build).toEqual("function");
        done();
    });

    it("creates a messaging stub and uses it correctly", done => {
        const mqttMessagingStub = mqttMessagingStubFactory.build(mqttAddress);
        mqttMessagingStub.transmit(joynrMessage).catch(() => {
            return null;
        });
        expect(mqttClient.send).toHaveBeenCalledWith(jasmine.any(String), joynrMessage);
        done();
    });
});