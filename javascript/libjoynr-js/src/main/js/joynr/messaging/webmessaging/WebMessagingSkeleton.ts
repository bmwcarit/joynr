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
import * as Typing from "../../util/Typing";
import * as UtilInternal from "../../util/UtilInternal";

class WebMessagingSkeleton {
    private settings: {
        window: Window;
    };

    private receiverCallbacks: Function[] = [];
    /**
     * @constructor WebMessagingSkeleton
     * @param settings the settings object for this constructor call
     * @param settings.window the window to register the event handler at
     */
    public constructor(settings: { window: Window }) {
        Typing.checkProperty(settings, "Object", "settings");

        if (settings.window === undefined) {
            throw new Error("WebMessagingSkeleton constructor parameter windows is undefined");
        }

        if (settings.window.addEventListener === undefined || settings.window.removeEventListener === undefined) {
            throw new Error(
                'WebMessagingSkeleton constructor parameter window does not provide the expected functions "addEventListener" and "removeEventListener"'
            );
        }

        this.callbackFct = this.callbackFct.bind(this);
        settings.window.addEventListener("message", this.callbackFct);

        this.settings = settings;
    }

    public callbackFct(event: any): void {
        UtilInternal.fire(this.receiverCallbacks, event.data);
    }

    /**
     * Registers a listener for web messaging
     *
     * @param listener the listener function receiving the messaging events events with the signature "function(joynrMessage) {..}"
     */
    public registerListener(listener: Function): void {
        Typing.checkPropertyIfDefined(listener, "Function", "listener");

        this.receiverCallbacks.push(listener);
    }

    /**
     * Unregisters a listener for web messaging
     *
     * @param listener the listener function receiving the messaging events events with the signature "function(joynrMessage) {..}"
     */
    public unregisterListener(listener: Function): void {
        Typing.checkPropertyIfDefined(listener, "Function", "listener");

        UtilInternal.removeElementFromArray(this.receiverCallbacks, listener);
    }

    public shutdown(): void {
        this.settings.window.removeEventListener("message", this.callbackFct);
    }
}

export = WebMessagingSkeleton;
