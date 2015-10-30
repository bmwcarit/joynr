/*jslint es5: true */

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

define(
        "joynr/util/MethodUtil",
        [
            "joynr/util/Typing",
            "joynr/TypesEnum"
        ],
        function(Typing, TypesEnum) {
            var MethodUtil = {};

            MethodUtil.transformParameterMapToArray =
                    function transformParameterMapToArray(operationArguments, parameter) {
                        var argument, argumentName, argumentValue, argumentId, operationParameter, transformedParameterList =
                                {
                                    params : [],
                                    paramDatatypes : []
                                };

                        // check if number of parameters in signature matches number of arguments
                        if (Object.keys(parameter).length !== Object.keys(operationArguments).length) {
                            throw new Error("signature does not match: wrong number of arguments");
                        }

                        for (argumentId in parameter) {
                            if (parameter.hasOwnProperty(argumentId)) {
                                // check if there's a parameter with the given name
                                argument = parameter[argumentId];

                                argumentName = argument.name;
                                operationParameter =
                                        (argument.type.substr(argument.type.length - 2, 2) === "[]")
                                                ? TypesEnum.LIST
                                                : argument.type;

                                // if there's no parameter with the given name
                                if (!operationParameter) {
                                    // signature does not match
                                    throw new Error(
                                            "signature does not match: type for argument \""
                                                + argumentName
                                                + "\" missing");
                                }

                                // retrieve the argument value
                                argumentValue = operationArguments[argumentName];

                                // if argument value is not given by the application
                                if (argumentValue === undefined || argumentValue === null) {
                                    throw new Error("Cannot call operation with nullable value \""
                                        + argumentValue
                                        + "\" of argument \""
                                        + argumentName
                                        + "\"");
                                }

                                // check if the parameter type matches the type of the argument value
                                /*jslint nomen: true */// allow dangling _ in variable once
                                var objectType =
                                        argumentValue._typeName
                                            || Typing.getObjectType(argumentValue);
                                /*jslint nomen: false */
                                if (Typing.translateJoynrTypeToJavascriptType(operationParameter) !== objectType) {
                                    // signature does not match
                                    throw new Error(
                                            "Signature does not match: type \""
                                                + objectType
                                                + "\" of argument \""
                                                + argumentName
                                                + "\" does not match with expected type \""
                                                + Typing
                                                        .translateJoynrTypeToJavascriptType(operationParameter)
                                                + "\"");
                                }

                                // we found a matching parameter/argument-pair that has the same name and
                                // type, let's add it to our qualified operation
                                // argument object for later use in serialization
                                transformedParameterList.paramDatatypes.push(operationParameter);
                                transformedParameterList.params.push(argumentValue);
                            }
                        }
                        return transformedParameterList;
                    };

            return MethodUtil;

        });