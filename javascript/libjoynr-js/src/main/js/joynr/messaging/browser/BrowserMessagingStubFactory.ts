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
import BrowserMessagingStub from "./BrowserMessagingStub";
import WebMessagingStub = require("../webmessaging/WebMessagingStub");

class BrowserMessagingStubFactory {
    private readonly webMessagingStub: WebMessagingStub;
    /**
     * @constructor
     *
     * @param settings
     * @param settings.webMessagingStub an initialized sender that has the default window already set
     */
    public constructor(settings: { webMessagingStub: WebMessagingStub }) {
        Typing.checkProperty(settings, "Object", "settings");
        Typing.checkProperty(settings.webMessagingStub, "WebMessagingStub", "settings.webMessagingStub");

        this.webMessagingStub = settings.webMessagingStub;
    }

    /**
     * @param address the address to generate a messaging stub for
     */
    public build(address: BrowserAddress): BrowserMessagingStub {
        return new BrowserMessagingStub({
            windowId: address.windowId,
            webMessagingStub: this.webMessagingStub
        });
    }
}

export = BrowserMessagingStubFactory;
