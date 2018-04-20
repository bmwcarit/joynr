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
const Typing = require("../../util/Typing");
const MqttMessagingStub = require("./MqttMessagingStub");

/**
 * @constructor
 * @name MqttMessagingStubFactory
 * @param {Object}
 *            settings
 * @param {SharedMqttClient}
 *            settings.client the mqtt client
 */
const MqttMessagingStubFactory = function MqttMessagingStubFactory(settings) {
    Typing.checkProperty(settings, "Object", "settings");
    Typing.checkProperty(settings.client, "SharedMqttClient", "client");
    this._settings = settings;
};

/**
 * @name MqttMessagingStubFactory#build
 * @function
 */
MqttMessagingStubFactory.prototype.build = function build(address) {
    Typing.checkProperty(address, "MqttAddress", "address");

    return new MqttMessagingStub({
        address,
        client: this._settings.client
    });
};

module.exports = MqttMessagingStubFactory;
