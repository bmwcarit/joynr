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
import * as JSONSerializer from "../../util/JSONSerializer";

import LoggingManager from "../../system/LoggingManager";
import JoynrMessage = require("../JoynrMessage");

const log = LoggingManager.getLogger("joynr/messaging/webmessaging/WebMessagingStub");

class WebMessagingStub {
    private settings: any;
    /**
     * @constructor
     *
     * @param settings the settings object for this constructor call
     * @param settings.window the default target window, the messages should be sent to
     * @param settings.origin the default origin, the messages should be sent to
     */
    public constructor(settings: { window: Record<string, any>; origin: string }) {
        this.settings = settings;
    }

    /**
     * @param message the message to transmit
     */
    public transmit(message: { message: JoynrMessage; windowId?: string }): Promise<void> {
        //TODO: check why sending a JoynrMessage provokes the following error
        // maybe enumerability or visibility of members while using Object.defineProperties
        /*
              DataCloneError: An object could not be cloned.
              code: 25
              message: "An object could not be cloned."
              name: "DataCloneError"
              stack: "Error: An object could not be cloned.
              __proto__: DOMException
              */
        log.debug(`transmit message: "${JSONSerializer.stringify(message)}"`);
        this.settings.window.postMessage(JSON.parse(JSONSerializer.stringify(message)), this.settings.origin);

        return Promise.resolve();
    }
}

export = WebMessagingStub;
