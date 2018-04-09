/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
 * @constructor
 * @name InProcessAddress
 *
 * @param {InProcessMessagingSkeleton} inProcessMessagingSkeleton the skeleton that should be addressed in process
 */
function InProcessAddress(inProcessMessagingSkeleton) {
    this._inProcessMessagingSkeleton = inProcessMessagingSkeleton;

    /**
     * @name InProcessAddress#_typeName
     * @type String
     * @readonly
     */
    Object.defineProperty(this, "_typeName", {
        configurable: false,
        writable: false,
        enumerable: false,
        value: InProcessAddress._typeName
    });
}

/**
 * @name InProcessAddress#_typeName
 * @type String
 * @readonly
 */
Object.defineProperty(InProcessAddress, "_typeName", {
    configurable: false,
    writable: false,
    enumerable: false,
    value: "joynr.system.RoutingTypes.InProcessAddress"
});

Object.defineProperty(InProcessAddress.prototype, "equals", {
    enumerable: false,
    configurable: false,
    writable: false,
    readable: true,
    value: function equals(other) {
        let i;
        if (this === other) {
            return true;
        }
        if (other === undefined || other === null) {
            return false;
        }
        if (other._typeName === undefined || this._typeName !== other._typeName) {
            return false;
        }
        if (this.getSkeleton() !== other.getSkeleton()) {
            return false;
        }
        return true;
    }
});

/**
 * The receive function from the corresponding local messaging receiver
 * @name InProcessAddress#getSkeleton
 * @function
 *
 * @returns {InProcessMessagingSkeleton} the skeleton that should be addressed
 */
InProcessAddress.prototype.getSkeleton = function getSkeleton() {
    return this._inProcessMessagingSkeleton;
};

module.exports = InProcessAddress;
