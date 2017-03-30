/* *
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

define([ "joynr/messaging/mqtt/MqttMessagingSkeleton"
], function(MqttMessagingSkeleton) {
    describe("libjoynr-js.joynr.messaging.mqtt.MqttMessagingSkeleton", function() {

        var sharedWebSocket, mqttMessagingSkeleton;

        beforeEach(function() {
            function SharedMqttClient() {
                this.subscribe = function() {};
            }

            function MessageRouter() {}

            function MqttAddress() {}

            sharedWebSocket = new SharedMqttClient();

            spyOn(sharedWebSocket, "subscribe");

            mqttMessagingSkeleton = new MqttMessagingSkeleton({
                address : new MqttAddress(),
                client : sharedWebSocket,
                messageRouter : new MessageRouter()
            });

            sharedWebSocket.subscribe.calls.reset();
        });

        function testCorrectMulticastIdTransformation(multicastId, expectedTopic) {
            mqttMessagingSkeleton.registerMulticastSubscription(multicastId);
            expect(sharedWebSocket.subscribe).toHaveBeenCalled();
            expect(sharedWebSocket.subscribe).toHaveBeenCalledWith(expectedTopic);
            sharedWebSocket.subscribe.calls.reset();
        }

        it("correctly transform multicastIds to mqtt topics", function() {
            testCorrectMulticastIdTransformation("a/b", "a/b");
            testCorrectMulticastIdTransformation("a/+/b", "a/+/b");
            testCorrectMulticastIdTransformation("a/*", "a/#");
            testCorrectMulticastIdTransformation("x/y/z/*", "x/y/z/#");
        });
    });
});
