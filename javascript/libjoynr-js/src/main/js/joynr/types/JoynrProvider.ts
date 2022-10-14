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
/* istanbul ignore file */

import JoynrObject from "./JoynrObject";
import { ProviderAttribute } from "../provider/ProviderAttribute";
import ProviderOperation from "../provider/ProviderOperation";

export interface ProviderReadAttributeImpl<T> {
    get: () => T | Promise<T>;
}

export interface ProviderWriteAttributeImpl<T> {
    set: (value: T) => void;
}

export interface ProviderNotifyAttributeImpl<T> {
    valueChanged: (value: T) => void;
}

export type ProviderReadWriteAttributeImpl<T> = ProviderReadAttributeImpl<T> & ProviderWriteAttributeImpl<T>;
export type ProviderReadNotifyAttributeImpl<T> = ProviderReadAttributeImpl<T> & ProviderNotifyAttributeImpl<T>;
export type ProviderReadWriteNotifyAttributeImpl<T> = ProviderReadAttributeImpl<T> &
    ProviderWriteAttributeImpl<T> &
    ProviderNotifyAttributeImpl<T>;

export abstract class JoynrProvider extends JoynrObject {
    public abstract interfaceName: string;
    protected constructor() {
        super();
    }

    public checkImplementation(): string[] {
        const missingInImplementation: string[] = [];
        Object.values(this).forEach(elem => {
            if (elem instanceof ProviderAttribute) {
                if (!elem.check()) {
                    missingInImplementation.push(elem.attributeName);
                }
            }
            if (elem instanceof ProviderOperation) {
                if (!elem.checkOperation()) {
                    missingInImplementation.push(elem.operationName);
                }
            }
        });

        return missingInImplementation;
    }
}

export interface JoynrProviderType extends Function {
    new (...args: any[]): JoynrProvider;
    MAJOR_VERSION: number;
    MINOR_VERSION: number;
    getUsedJoynrtypes: any[];
}
