/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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

/**
 * @name GenerationUtil
 * @class
 */
const GenerationUtil = {};

function equalsCompound(other) {
    let key, member, otherMember, hasEquals, length, i;
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

    for (key in this) {
        if (this.hasOwnProperty(key)) {
            member = this[key];
            otherMember = other[key];
            if (otherMember === undefined || otherMember === null) {
                return false;
            }

            if (Array.isArray(member)) {
                if (member.length !== otherMember.length) {
                    return false;
                }
                // check only the first function for equals
                hasEquals = member[0].equals && typeof member[0].equals === "function";
                length = member.length;

                if (hasEquals) {
                    for (i = 0; i < length; i++) {
                        if (!member[i].equals(otherMember[i])) {
                            return false;
                        }
                    }
                } else {
                    for (i = 0; i < length; i++) {
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

function equalsEnum(other) {
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

/**
 *
 * @param compoundJoynrObject
 *      adds the equals implementation to a complex joynr Object
 */
GenerationUtil.addEqualsCompound = function(compoundJoynrObject) {
    Object.defineProperty(compoundJoynrObject.prototype, "equals", {
        enumerable: false,
        configurable: false,
        writable: false,
        readable: true,
        value: equalsCompound
    });
};

/**
 *
 * @param enumJoynrObject
 *      adds the equals implementation to the prototype of an enum type
 */
GenerationUtil.addEqualsEnum = function(enumJoynrObject) {
    Object.defineProperty(enumJoynrObject.prototype, "equals", {
        enumerable: false,
        configurable: false,
        writable: false,
        readable: true,
        value: equalsEnum
    });
};

function getMemberType(memberName) {
    if (this._memberTypes[memberName] !== undefined) {
        return this._memberTypes[memberName];
    }
}

GenerationUtil.addMemberTypeGetter = function(joynrObject) {
    Object.defineProperty(joynrObject, "getMemberType", {
        value: getMemberType
    });
};

GenerationUtil.addMapUtility = function(joynrObject, propertyTypeName) {
    joynrObject.prototype.put = function(key, value) {
        this[key] = value;
    };

    joynrObject.prototype.get = function(key) {
        return this[key];
    };

    joynrObject.prototype.remove = function(key) {
        delete this[key];
    };

    Object.defineProperty(joynrObject, "checkMembers", {
        value: function checkMembers(instance, check) {
            let memberKey;
            for (memberKey in instance) {
                if (instance.hasOwnProperty(memberKey)) {
                    if (memberKey !== "_typeName") {
                        check(instance[memberKey], propertyTypeName, memberKey);
                    }
                }
            }
        }
    });
};

module.exports = GenerationUtil;
