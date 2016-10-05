/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

define(
        "joynr/messaging/mqtt/MqttMessagingStubFactory",
        [
            "joynr/util/Util",
            "joynr/messaging/mqtt/MqttMessagingStub",
            "joynr/system/RoutingTypes/MqttAddress"
        ],
        function(Util, MqttMessagingStub, MqttAddress) {

            /**
             * @constructor
             * @name MqttMessagingStubFactory
             * @param {Object}
             *            settings
             * @param {SharedMqttClient}
             *            settings.client the mqtt client
             * @param {MqttAddress}
             *            settings.address 
             * @param {MqttMessageReplyToAddressCalculator} mqttMessageReplyToAddressCalculator calculates the replyTo address
             */
            var MqttMessagingStubFactory =
                    function MqttMessagingStubFactory(settings) {
                        Util.checkProperty(settings, "Object", "settings");
                        Util.checkProperty(settings.address, "MqttAddress", "address");
                        Util.checkProperty(settings.client, "SharedMqttClient", "client");
                        Util.checkProperty(
                                settings.mqttMessageReplyToAddressCalculator,
                                "MqttMessageReplyToAddressCalculator",
                                "mqttMessageReplyToAddressCalculator");

                        /**
                         * @name MqttMessagingStubFactory#build
                         * @function
                         */
                        this.build =
                                function build(address) {
                                    Util.checkProperty(address, "MqttAddress", "address");

                                    return new MqttMessagingStub(
                                            {
                                                address : address,
                                                client : settings.client,
                                                mqttMessageReplyToAddressCalculator : settings.mqttMessageReplyToAddressCalculator
                                            });
                                };
                    };

            return MqttMessagingStubFactory;

        });