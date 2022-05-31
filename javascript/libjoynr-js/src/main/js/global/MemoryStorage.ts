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
import { Storage } from "./interface/Persistency";

class MemoryStorage implements Storage {
    public map: Record<string, any> = {};
    /**
     * MemoryStorage has the same Interface as LocalStorage and is used when persistency is disabled,
     * but the logic relies on the persistency.
     * @constructor
     */
    public constructor() {
        // do nothing
    }

    public setItem(key: string, value: any): void {
        this.map[key] = value;
    }

    public getItem(key: string): any {
        return this.map[key];
    }

    public removeItem(key: string): void {
        delete this.map[key];
    }

    public clear(): void {
        this.map = {};
    }
}

export = MemoryStorage;
