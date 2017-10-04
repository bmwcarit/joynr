/*jslint es5: true, nomen: true, node: true */

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
var Typing = require('../../util/Typing');
var ChannelMessagingStub = require('./ChannelMessagingStub');

/**
 * @constructor
 * @name ChannelMessagingStubFactory
 *
 * @param {Object} settings
 * @param {ChannelMessagingSender|Object} settings.channelMessagingSender
 */
function ChannelMessagingStubFactory(settings) {

    Typing.checkProperty(settings, "Object", "settings");
    Typing.checkProperty(
            settings.channelMessagingSender,
            "ChannelMessagingSender",
            "channelMessagingSender");
    this._globalAddress = null;
    this._settings = settings;
}

/**
 * This method is called when the global address has been created
 *
 * @function
 * @name CapabilityDiscovery#globalAddressReady
 *
 * @param {Address}
 *            globalAddress the address used to register discovery entries globally
 */
ChannelMessagingStubFactory.prototype.globalAddressReady =
        function globalAddressReady(newGlobalAddress) {
            this._globalAddress = newGlobalAddress;
        };

/**
 * @name ChannelMessagingStubFactory#build
 * @function
 *
 * @param {ChannelAddress} address the address to generate a messaging stub for
 */
ChannelMessagingStubFactory.prototype.build = function build(address) {
    Typing.checkProperty(address, "ChannelAddress", "address");
    if (!this._globalAddress) {
        var error = new Error("global channel address not yet set");
        error.delay = true;
        throw error;
    }
    return new ChannelMessagingStub({
        destinationChannelAddress : address,
        channelMessagingSender : this._settings.channelMessagingSender,
        myChannelAddress : this._globalAddress
    });
};

module.exports = ChannelMessagingStubFactory;
