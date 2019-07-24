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
import * as Typing from "../joynr/util/Typing";
import { Persistency, PersistencySettings } from "./interface/Persistency";

class LocalStorage implements Persistency {
    /**
     * constructor for a localStorage object
     * @param settings the settings object
     * @param settings.clearPersistency localStorage is cleared if set to true
     */
    public constructor(settings: PersistencySettings) {
        settings = settings || {};
        Typing.checkPropertyIfDefined(settings.clearPersistency, "Boolean", "settings.clearPersistency");
        if (settings.clearPersistency) {
            localStorage.clear();
        }
    }

    public setItem(key: string, value: any): void {
        localStorage.setItem(key, value);
    }
    public getItem(key: string): any {
        return localStorage.getItem(key);
    }
    public removeItem(key: string): void {
        localStorage.removeItem(key);
    }
    public clear(): void {
        localStorage.clear();
    }
    public init(): Promise<void> {
        return Promise.resolve();
    }
    public shutdown(): Promise<void> {
        return Promise.resolve();
    }
}

export = LocalStorage;
