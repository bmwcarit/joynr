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
import { checkProperty } from "../util/UtilInternal";
import JoynrRuntimeException from "./JoynrRuntimeException";

class PublicationMissedException extends JoynrRuntimeException {
    public subscriptionId: string;

    public name = "";
    /**
     * Used for serialization.
     */
    public _typeName = "joynr.exceptions.PublicationMissedException";

    /**
     * Constructor of PublicationMissedException object used to report
     * when a publication has not been received within the expected
     * time period.
     *
     * @param [settings] the settings object for the constructor call
     * @param [settings.detailMessage] message containing details
     *            about the error
     * @param [settings.subscriptionId] the id of the subscription
     */
    public constructor(settings: { detailMessage: string; subscriptionId: string }) {
        super(settings);
        Object.defineProperty(this, "name", {
            enumerable: false,
            configurable: false,
            writable: true,
            value: "PublicationMissedException"
        });

        checkProperty(settings.subscriptionId, "String", "settings.subscriptionId");

        this.subscriptionId = settings.subscriptionId;
    }

    public static _typeName = "joynr.exceptions.PublicationMissedException";
}

export = PublicationMissedException;
