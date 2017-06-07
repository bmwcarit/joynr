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

define("joynr/messaging/mqtt/MqttMessagingSkeleton", [
    "joynr/util/UtilInternal",
    "joynr/util/Typing",
    "joynr/types/TypeRegistrySingleton"
], function(Util, Typing, TypeRegistrySingleton) {

    var typeRegistry = TypeRegistrySingleton.getInstance();

    /**
     * @constructor MqttMessagingSkeleton
     * @param {Object} settings
     * @param {SharedMqttClient} settings.client the mqtt client to be used to transmit messages
     * @param {MessageRouter} settings.messageRouter the message router
     * @param {MqttAddress} settings.address own address of joynr client
     */
    var MqttMessagingSkeleton =
            function MqttMessagingSkeleton(settings) {
                Typing.checkProperty(settings, "Object", "settings");
                Typing.checkProperty(settings.client, "SharedMqttClient", "settings.client");
                Typing.checkProperty(
                        settings.messageRouter,
                        "MessageRouter",
                        "settings.messageRouter");
                Typing.checkProperty(settings.address, "MqttAddress", "settings.address");

                var multicastSubscriptionCount = {};
                // because the message is received via global transport, isGloballyVisible must be true
                var isGloballyVisible = true;

                settings.client.onmessage =
                        function(topic, message) {
                            message.setReceivedFromGlobal(true);
                            var replyToMqttAddress = message.replyChannelId;
                            if (!Util.checkNullUndefined(replyToMqttAddress)) {
                                settings.messageRouter.addNextHop(
                                        message.from,
                                        Typing.augmentTypes(
                                                JSON.parse(replyToMqttAddress),
                                                typeRegistry),
                                        isGloballyVisible);
                            }
                            settings.messageRouter.route(message);
                        };

                settings.client.subscribe(settings.address.topic + "/#");

                function translateWildcard(multicastId) {
                    if (multicastId.match(/[\w\W]*\/[*]$/)) {
                        return multicastId.replace(/\/\*/g, '/#');
                    }
                    return multicastId;
                }

                this.registerMulticastSubscription =
                        function(multicastId) {
                            if (multicastSubscriptionCount[multicastId] === undefined) {
                                multicastSubscriptionCount[multicastId] = 0;
                            }
                            settings.client.subscribe(translateWildcard(multicastId));
                            multicastSubscriptionCount[multicastId] =
                                    multicastSubscriptionCount[multicastId] + 1;
                        };

                this.unregisterMulticastSubscription = function(multicastId) {
                    var subscribersCount = multicastSubscriptionCount[multicastId];
                    if (subscribersCount !== undefined) {
                        subscribersCount--;
                        if (subscribersCount === 0) {
                            settings.client.unsubscribe(translateWildcard(multicastId));
                            delete multicastSubscriptionCount[multicastId];
                        } else {
                            multicastSubscriptionCount[multicastId] = subscribersCount;
                        }
                    }
                };

                /**
                 * @function MqttMessagingSkeleton#shutdown
                 */
                this.shutdown = function shutdown() {
                    settings.client.close();
                };
            };

    return MqttMessagingSkeleton;

});