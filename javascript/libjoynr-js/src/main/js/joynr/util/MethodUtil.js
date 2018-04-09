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
const Typing = require("./Typing");
const Util = require("./UtilInternal");

/**
 * @exports MethodUtil
 */
const MethodUtil = {};

/**
 * @param {Object[]} operationArguments Arguments coming from the proxy or provider
 * @param {Object[]} parameters List of paraemters for the operationArguments
 * @returns {Object} Object in the form of { paramDatatypes: [Array of types], params: [Array of values] };
 */
MethodUtil.transformParameterMapToArray = function transformParameterMapToArray(operationArguments, parameters) {
    let argument,
        objectType,
        argumentId,
        argumentValue,
        params = [],
        paramDatatypes = [];

    // check if number of parameters in signature matches number of arguments
    if (Object.keys(parameters).length !== Object.keys(operationArguments).length) {
        throw new Error("signature does not match: wrong number of arguments");
    }

    for (argumentId = 0; argumentId < parameters.length; argumentId++) {
        // check if there's a parameters with the given name
        argument = parameters[argumentId];
        // retrieve the argument value
        argumentValue = operationArguments[argument.name];
        // if argument value is not given by the application
        if (Util.checkNullUndefined(argumentValue)) {
            throw new Error(
                'Cannot call operation with nullable value "' + argumentValue + '" of argument "' + argument.name + '"'
            );
        }
        // check if the parameter type matches the type of the argument value
        // allow dangling _ in variable once
        /*jslint nomen: true */
        objectType = Array.isArray(argumentValue) ? "Array" : argumentValue._typeName || typeof argumentValue;
        /*jslint nomen: false */
        if (argument.javascriptType !== objectType) {
            // signature does not match
            throw new Error(
                'Signature does not match: type "' +
                    objectType +
                    '" of argument "' +
                    argument.name +
                    '" does not match with expected type "' +
                    argument.javascriptType +
                    '"'
            );
        }

        paramDatatypes.push(argument.type);
        params.push(argumentValue);
    }
    return {
        paramDatatypes,
        params
    };
};

module.exports = MethodUtil;
