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
import JoynrMessage from "../JoynrMessage";

import * as Typing from "../../util/Typing";
import * as UtilInternal from "../../util/UtilInternal";
import * as JSONSerializer from "../../util/JSONSerializer";
import LoggingManager from "../../system/LoggingManager";
import WebMessagingSkeleton = require("../webmessaging/WebMessagingSkeleton");

class BrowserMessagingSkeleton {
    public receiverCallbacks: any;
    private log: any;
    /**
     * @constructor BrowserMessagingSkeleton
     *
     * @param settings
     * @param settings.webMessagingSkeleton a web messaging skeleton receiving web messages
     */
    public constructor(settings: { webMessagingSkeleton: WebMessagingSkeleton }) {
        this.log = LoggingManager.getLogger("joynr/messaging/browser/BrowserMessagingSkeleton");
        Typing.checkProperty(settings, "Object", "settings");
        Typing.checkProperty(settings.webMessagingSkeleton, Object, "settings.webMessagingSkeleton");

        this.receiverCallbacks = [];

        this.webMessagingSkeletonListener = this.webMessagingSkeletonListener.bind(this);
        settings.webMessagingSkeleton.registerListener(this.webMessagingSkeletonListener);
    }

    /**
     * Registers the listener function
     *
     * @param listener a listener function that should be added and should receive messages
     */
    public registerListener(listener: Function): void {
        Typing.checkProperty(listener, "Function", "listener");

        this.receiverCallbacks.push(listener);
    }

    /**
     * Unregisters the listener function
     *
     * @param listener the listener function that should re removed and shouldn't receive messages any more
     */
    public unregisterListener(listener: Function): void {
        Typing.checkProperty(listener, "Function", "listener");

        UtilInternal.removeElementFromArray(this.receiverCallbacks, listener);
    }

    private webMessagingSkeletonListener(message: JoynrMessage): void {
        if (message !== undefined) {
            const joynrMessage = new JoynrMessage(message);

            UtilInternal.fire(this.receiverCallbacks, joynrMessage);
        } else {
            this.log.warn(`message with content "${JSONSerializer.stringify(message)}" could not be processed`);
        }
    }
}

export = BrowserMessagingSkeleton;
