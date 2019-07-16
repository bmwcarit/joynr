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

class WebMessagingAddress {
    private settings: any;
    /**
     * @constructor WebMessagingAddress
     * @param settings the settings object for this constructor call
     * @param settings.window the default target window, the messages should be sent to
     * @param settings.origin the default origin, the messages should be sent to
     */
    public constructor(settings: { window: Window; origin: string }) {
        Typing.checkProperty(settings, "Object", "settings");
        Typing.checkProperty(settings.window, "Object", "settings.window");
        Typing.checkProperty(settings.origin, "String", "settings.origin");
        this.settings = settings;
    }

    /**
     * @returns the windows that should be addressed
     */
    public getWindow(): Window {
        return this.settings.window;
    }

    /**
     * @returns the origin of the window that should be addressed
     * @see WebMessagingAddress#getWindow
     */
    public getOrigin(): string {
        return this.settings.origin;
    }
}

export = WebMessagingAddress;
