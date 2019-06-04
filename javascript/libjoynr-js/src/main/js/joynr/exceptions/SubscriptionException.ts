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

class SubscriptionException extends JoynrRuntimeException {
    public subscriptionId: string;
    public name = "SubscriptionException";

    /**
     * Used for serialization.
     */
    public _typeName = "joynr.exceptions.SubscriptionException";

    /**
     * Constructor of SubscriptionException object used for reporting
     * error conditions when creating a subscription (e.g. the
     * provided subscription parameters are not correct etc.) that should
     * be transmitted back to consumer side.
     *
     * @param settings - the settings object for the constructor call
     * @param [settings.detailMessage] message containing details
     *            about the error
     * @param settings.subscriptionId - Id of the subscription
     */
    public constructor(settings: { detailMessage: string; subscriptionId: string }) {
        super(settings);
        this.subscriptionId = settings && settings.subscriptionId;
    }

    public static _typeName = "joynr.exceptions.SubscriptionException";
}

export = SubscriptionException;
