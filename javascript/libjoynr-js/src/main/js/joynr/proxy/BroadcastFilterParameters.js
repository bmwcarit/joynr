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
const Typing = require("../util/Typing");
const Util = require("../util/UtilInternal");
const LoggingManager = require("../system/LoggingManager");

function makeSetterFunction(obj, parameterName) {
    return function(arg) {
        obj.filterParameters[parameterName] = arg;
        return obj;
    };
}

/**
 * Constructor of BroadcastFilterParameters object used for subscriptions in generated proxy objects
 *
 * @constructor
 * @name BroadcastFilterParameters
 *
 * @param {Object}
 *            [filterParameters] the filterParameters object for the constructor call
 *
 * @returns {BroadcastFilterParameters} a BroadcastFilterParameters Object for subscriptions on broadcasts
 */
function BroadcastFilterParameters(filterParameterProperties) {
    if (!(this instanceof BroadcastFilterParameters)) {
        // in case someone calls constructor without new keyword (e.g. var c
        // = Constructor({..}))
        return new BroadcastFilterParameters(filterParameterProperties);
    }

    const log = LoggingManager.getLogger("joynr.proxy.BroadcastFilterParameters");

    /**
     * @name BroadcastFilterParameters#_typeName
     * @type String
     */
    Util.objectDefineProperty(this, "_typeName", "joynr.BroadcastFilterParameters");
    Typing.checkPropertyIfDefined(filterParameterProperties, "Object", "filterParameters");

    let parameterName;
    let funcName;

    if (filterParameterProperties === undefined) {
        let filterParameters = null;
        Object.defineProperty(this, "filterParameters", {
            readable: true,
            enumerable: true,
            configurable: false,
            get() {
                return filterParameters;
            },
            set(value) {
                filterParameters = value;
            }
        });
    } else {
        for (parameterName in filterParameterProperties) {
            if (filterParameterProperties.hasOwnProperty(parameterName)) {
                funcName = "set" + parameterName.charAt(0).toUpperCase() + parameterName.substring(1);
                //filter[funcName] = makeSetterFunction(filter, parameterName);
                Object.defineProperty(this, funcName, {
                    configurable: false,
                    writable: false,
                    enumerable: false,
                    value: makeSetterFunction(this, parameterName)
                });
            }
        }

        /**
         * @name BroadcastFilterParameters#filterParameters
         * @type Object
         */
        this.filterParameters = {};
    }
}

module.exports = BroadcastFilterParameters;
