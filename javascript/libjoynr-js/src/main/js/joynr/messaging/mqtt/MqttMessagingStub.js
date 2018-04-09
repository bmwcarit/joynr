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
const JoynrMessage = require("../JoynrMessage");
const JSONSerializer = require("../../util/JSONSerializer");
const LoggingManager = require("../../system/LoggingManager");

const log = LoggingManager.getLogger("joynr/messaging/mqtt/MqttMessagingStub");
/**
 * @name MqttMessagingStub
 * @constructor

 * @param {Object} settings the settings object for this constructor call
 * @param {MqttAddress} settings.address the mqtt address of the message destination
 * @param {SharedMqttClient} settings.client the mqtt client to be used to transmit messages
 */
function MqttMessagingStub(settings) {
    this._settings = settings;
}

/**
 * @name MqttMessagingStub#transmit
 * @function
 *
 * @param {Object|JoynrMessage} message the message to transmit
 */
MqttMessagingStub.prototype.transmit = function transmit(message) {
    log.debug('transmit message: "' + JSONSerializer.stringify(message) + '"');
    let topic = this._settings.address.topic;
    if (!(JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST === message.type)) {
        topic += MqttMessagingStub.PRIORITY_LOW + message.to;
    }

    return this._settings.client.send(topic, message);
};

MqttMessagingStub.PRIORITY_LOW = "/low/";

module.exports = MqttMessagingStub;
