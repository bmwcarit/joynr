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

import JoynrObject = require("./JoynrObject");

abstract class JoynrCompound extends JoynrObject {
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

        if (Object.keys(this).length !== Object.keys(other).length) {
            return false;
        }

        for (const key in this) {
            if (Object.prototype.hasOwnProperty.call(this, key)) {
                const member: any = this[key];
                const otherMember = other[key];
                if (otherMember === undefined || otherMember === null) {
                    return false;
                }

                if (Array.isArray(member)) {
                    if (member.length !== otherMember.length) {
                        return false;
                    }
                    // check only the first function for equals
                    const hasEquals = member[0] && member[0].equals && typeof member[0].equals === "function";
                    const length = member.length;

                    if (hasEquals) {
                        for (let i = 0; i < length; i++) {
                            if (!member[i].equals(otherMember[i])) {
                                return false;
                            }
                        }
                    } else {
                        for (let i = 0; i < length; i++) {
                            if (member[i] !== otherMember[i]) {
                                return false;
                            }
                        }
                    }
                } else if (member.equals && typeof member.equals === "function") {
                    if (!member.equals(otherMember)) {
                        return false;
                    }
                } else if (member !== otherMember) {
                    return false;
                }
            }
        }
        return true;
    }
}
export = JoynrCompound;
