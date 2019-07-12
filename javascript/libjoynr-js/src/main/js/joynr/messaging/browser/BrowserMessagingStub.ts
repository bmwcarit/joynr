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

import WebMessagingStub = require("../webmessaging/WebMessagingStub");
import JoynrMessage = require("../JoynrMessage");

class BrowserMessagingStub {
    private settings: {
        windowId?: string;
        webMessagingStub: WebMessagingStub;
    };
    /**
     * @constructor
     *
     * @param settings
     * @param [settings.windowId] the destination windowId to send the messages to, defaults to defaultWindowId of master tab
     * @param settings.webMessagingStub an initialized sender that has the default window already set
     */
    public constructor(settings: { windowId?: string; webMessagingStub: WebMessagingStub }) {
        this.settings = settings;
    }

    /**
     * @param joynrMessage the joynr message to transmit
     */
    public transmit(joynrMessage: JoynrMessage): Promise<void> {
        return this.settings.webMessagingStub.transmit({
            windowId: this.settings.windowId,
            message: joynrMessage
        });
    }
}

export = BrowserMessagingStub;
