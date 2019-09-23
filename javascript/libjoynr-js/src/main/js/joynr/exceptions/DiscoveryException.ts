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
import JoynrRuntimeException from "./JoynrRuntimeException";

class DiscoveryException extends JoynrRuntimeException {
    public name = "";
    /**
     * Used for serialization.
     */
    public _typeName = "joynr.exceptions.DiscoveryException";
    /**
     * Constructor of DiscoveryException object used for reporting
     * error conditions during discovery and arbitration.
     *
     * @param [settings] the settings object for the constructor call
     * @param [settings.detailMessage] message containing details
     *            about the error
     */
    public constructor(settings: { detailMessage: string }) {
        super(settings);
        Object.defineProperty(this, "name", {
            enumerable: false,
            configurable: false,
            writable: true,
            value: "DiscoveryException"
        });
    }

    public static _typeName = "joynr.exceptions.DiscoveryException";
}

export = DiscoveryException;
