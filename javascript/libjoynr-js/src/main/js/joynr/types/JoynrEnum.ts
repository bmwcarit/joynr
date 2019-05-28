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

abstract class JoynrEnum<T> extends JoynrObject {
    protected constructor(public name: T, public value: T | number) {
        super();
    }

    public abstract _typeName: string;

    public equals(other: any): boolean {
        if (this === other) {
            return true;
        }
        if (other === undefined || other === null) {
            return false;
        }
        if (other._typeName === undefined || this._typeName !== other._typeName) {
            return false;
        }
        if (this.name !== other.name || this.value !== other.value) {
            return false;
        }
        return true;
    }
}
export = JoynrEnum;
