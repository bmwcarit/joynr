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

define("joynr/messaging/mqtt/MqttMessagingStub", [
    "global/Promise",
    "global/Mqtt",
    "joynr/messaging/JoynrMessage",
    "joynr/messaging/MessagingQosEffort",
    "joynr/util/UtilInternal",
    "joynr/util/JSONSerializer",
    "joynr/system/LoggerFactory"
], function(Promise, Mqtt, JoynrMessage, MessagingQosEffort, Util, JSONSerializer, LoggerFactory) {

    /**
     * @name MqttMessagingStub
     * @constructor

     * @param {Object} settings the settings object for this constructor call
     * @param {MqttAddress} settings.address the mqtt address of the message destination
     * @param {SharedMqttClient} settings.client the mqtt client to be used to transmit messages
     * @param {MessageReplyToAddressCalculator} messageReplyToAddressCalculator calculates the replyTo address
     */
    function MqttMessagingStub(settings) {
        var log = LoggerFactory.getLogger("joynr/messaging/mqtt/MqttMessagingStub");

        Util.checkProperty(settings, "Object", "settings");

        if (settings.client === undefined) {
            throw new Error("MqttMessagingStub constructor parameter \"client\" is undefined");
        }

        Util.checkProperty(settings.address, "MqttAddress", "settings.address");
        Util.checkProperty(settings.client, "SharedMqttClient", "settings.client");
        Util.checkProperty(
                settings.messageReplyToAddressCalculator,
                "MessageReplyToAddressCalculator",
                "settings.messageReplyToAddressCalculator");

        /**
         * @name MqttMessagingStub#transmit
         * @function
         *
         * @param {Object|JoynrMessage} message the message to transmit
         */
        this.transmit = function transmit(message) {
            settings.messageReplyToAddressCalculator.setReplyTo(message);

            log.debug("transmit message: \"" + JSONSerializer.stringify(message) + "\"");
            var topic = settings.address.topic;
            if (!(JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST === message.type)) {
                topic += MqttMessagingStub.PRIORITY_LOW + message.to;
            }

            var qosLevel = MqttMessagingStub.DEFAULT_QOS_LEVEL;
            if (MessagingQosEffort.BEST_EFFORT === message.effort) {
                qosLevel = MqttMessagingStub.BEST_EFFORT_QOS_LEVEL;
            }

            return settings.client.send(topic, message, qosLevel);
        };

    }

    MqttMessagingStub.DEFAULT_QOS_LEVEL = 1;
    MqttMessagingStub.BEST_EFFORT_QOS_LEVEL = 0;

    MqttMessagingStub.PRIORITY_LOW = "/low/";

    return MqttMessagingStub;

});