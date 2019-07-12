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
import * as BrowserAddress from "../../../generated/joynr/system/RoutingTypes/BrowserAddress";
import * as Typing from "../../util/Typing";
import JoynrMessage = require("../JoynrMessage");

class BrowserMulticastAddressCalculator {
    private settings: any;
    /**
     * @constructor BrowserMulticastAddressCalculator
     * @param settings
     * @param settings.globalAddress
     */
    public constructor(settings: { globalAddress: BrowserAddress }) {
        Typing.checkProperty(settings, "Object", "settings");
        Typing.checkProperty(settings.globalAddress, "BrowserAddress", "settings.globalAddress");
        this.settings = settings;
    }

    /**
     * Calculates the multicast address for the submitted joynr message
     *
     * @param _message
     * @return {Address} the multicast address
     */
    public calculate(_message: JoynrMessage): BrowserAddress {
        return this.settings.globalAddress;
    }
}

export = BrowserMulticastAddressCalculator;
