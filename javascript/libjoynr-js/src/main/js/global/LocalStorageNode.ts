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
import { Persistency, PersistencySettings } from "./interface/Persistency";
import JoynrStorage from "./JoynrPersist";

import LoggingManager from "../joynr/system/LoggingManager";
const log = LoggingManager.getLogger("joynr.global.localStorageNode");

class LocalStorageWrapper implements Persistency {
    private settings: any;
    private promiseChain: any;
    private map: Map<string, any>;
    private storage: JoynrStorage;

    /**
     * LocalStorage constructor (node wrapper for LocalStorage)
     * @constructor LocalStorageWrapper
     * @classdesc node wrapper for LocalStorage
     *
     * @param settings the settings object
     * @param settings.clearPersistency localStorage is cleared if set to true
     * @param settings.location optional, passed on to node-persist LocalStorage constructor
     */
    public constructor(settings: PersistencySettings) {
        settings = settings || {};
        this.storage = new JoynrStorage({
            dir: settings.location || "./localStorageStorage"
        });
        this.map = new Map();
        this.promiseChain = Promise.resolve();
        this.settings = settings;
    }
    public setItem(key: string, value: any): void {
        this.map.set(key, value);
        this.wrapFunction(this.storage.setItem.bind(this.storage), key, value);
    }
    public getItem(key: string): any {
        return this.map.get(key);
    }
    public removeItem(key: string): void {
        this.map.delete(key);
        this.wrapFunction(this.storage.removeItem.bind(this.storage), key);
    }
    public clear(): void {
        this.map.clear();
        this.wrapFunction(this.storage.clear.bind(this.storage));
    }

    public async init(): Promise<void> {
        const storageData = await this.storage.init();

        if (this.settings.clearPersistency) {
            return await this.storage.clear();
        }
        for (let i = 0, length = storageData.length; i < length; i++) {
            const storageObject = storageData[i];
            if (storageObject && storageObject.key) {
                this.map.set(storageObject.key, storageObject.value);
            }
        }
    }

    public async shutdown(): Promise<void> {
        await this.promiseChain;
    }
    private wrapFunction(cb: Function, ...args: any[]): void {
        this.promiseChain = this.promiseChain
            .then(() => {
                // eslint-disable-next-line promise/no-callback-in-promise
                return cb(...args);
            })
            .catch((e: any) => {
                log.error(`failure executing ${cb} with args ${JSON.stringify(args)} error: ${e}`);
            });
    }
}

export = LocalStorageWrapper;
