/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
/*eslint no-unused-vars: "off"*/
const Typing = require("../../util/Typing");

class BrowserMulticastAddressCalculator {
    /**
     * @constructor BrowserMulticastAddressCalculator
     * @param {Object} settings
     * @param {BrowserAddress} settings.globalAddress
     */
    constructor(settings) {
        Typing.checkProperty(settings, "Object", "settings");
        Typing.checkProperty(settings.globalAddress, "BrowserAddress", "settings.globalAddress");
        this._settings = settings;
    }

    /**
     * Calculates the multicast address for the submitted joynr message
     * @function BrowserMulticastAddressCalculator#calculate
     *
     * @param {JoynrMessage} message
     * @return {Address} the multicast address
     */
    calculate(message) {
        return this._settings.globalAddress;
    }
}

module.exports = BrowserMulticastAddressCalculator;
