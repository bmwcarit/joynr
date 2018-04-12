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
const UtilInternal = require("../util/UtilInternal");

function makeSetterFunction(obj, pos) {
    return function(arg) {
        obj.outputParameters[pos] = arg;
        return obj;
    };
}
function makeGetterFunction(obj, pos) {
    return function() {
        return obj.outputParameters[pos];
    };
}

/**
 * Constructor of BroadcastOutputParameters object used for subscriptions in generated provider objects
 *
 * @constructor
 * @name BroadcastOutputParameters
 *
 * @param {Object}
 *            [outputParameters] the outputParameters object for the constructor call
 *
 * @returns {BroadcastOutputParameters} a BroadcastOutputParameters Object for subscriptions on broadcasts
 */
function BroadcastOutputParameters(outputParameterProperties) {
    if (!(this instanceof BroadcastOutputParameters)) {
        // in case someone calls constructor without new keyword (e.g. var c
        // = Constructor({..}))
        return new BroadcastOutputParameters(outputParameterProperties);
    }

    /**
     * @name BroadcastOutputParameters#_typeName
     * @type String
     */
    UtilInternal.objectDefineProperty(this, "_typeName", "joynr.BroadcastOutputParameters");
    Typing.checkPropertyIfDefined(outputParameterProperties, "Array", "outputParameters");

    let parameterName;
    let setterFuncName;
    let getterFuncName;
    let i;

    //for (parameterName in outputParameterProperties) {
    for (i = 0; i < outputParameterProperties.length; i++) {
        if (outputParameterProperties[i].hasOwnProperty("name")) {
            parameterName = outputParameterProperties[i].name;
            setterFuncName = "set" + parameterName.charAt(0).toUpperCase() + parameterName.substring(1);
            //output[funcName] = makeSetterFunction(output, parameterName);
            Object.defineProperty(this, setterFuncName, {
                configurable: false,
                writable: false,
                enumerable: false,
                value: makeSetterFunction(this, i)
            });
            getterFuncName = "get" + parameterName.charAt(0).toUpperCase() + parameterName.substring(1);
            Object.defineProperty(this, getterFuncName, {
                configurable: false,
                writable: false,
                enumerable: false,
                value: makeGetterFunction(this, i)
            });
        }
    }

    /**
     * @name BroadcastOutputParameters#outputParameters
     * @type Array
     */
    this.outputParameters = [];
}

module.exports = BroadcastOutputParameters;
