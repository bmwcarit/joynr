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
import Address = require("../../generated/joynr/system/RoutingTypes/Address");
import * as UtilInternal from "../util/UtilInternal";
import LoggingManager from "../system/LoggingManager";

const log = LoggingManager.getLogger("joynr/messaging/MessagingStubFactory");

interface InternalMessagingStubFactory {
    build(address: Address): MessagingStub;
}

type MessagingStub = any;

class MessagingStubFactory {
    private messagingStubFactories: Record<string, InternalMessagingStubFactory>;
    /**
     * @constructor
     *
     * @param settings
     * @param settings.messagingStubFactories a hash mapping addresses to the equivalent message sender
     * @param settings.messagingStubFactories.KEY the key of the map is the className of the address provided in createMessagingStub, the value is the concrete MessagingStubFactory
     * @param settings.messagingStubFactories.KEY.build the factory method of the
     */
    public constructor(settings: { messagingStubFactories: Record<string, InternalMessagingStubFactory> }) {
        this.messagingStubFactories = settings.messagingStubFactories;
    }

    /**
     * @param address the address to create a messaging stub for
     */
    public createMessagingStub(address: Address): void {
        const className = address._typeName;
        const factory = this.messagingStubFactories[className];

        if (UtilInternal.checkNullUndefined(factory)) {
            const errorMsg = `Could not find a MessagingStubFactory for "${className}" within messagingStubFactories [${Object.keys(
                this.messagingStubFactories
            ).join(",")}]`;
            log.debug(errorMsg);
            throw new Error(errorMsg);
        }

        return factory.build(address);
    }
}

export = MessagingStubFactory;
