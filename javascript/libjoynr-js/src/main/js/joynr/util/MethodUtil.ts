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
import * as UtilInternal from "./UtilInternal";

function createErrorMessage(objectType: string, name: string, type: string): string {
    return `Signature does not match: type ${objectType} of argument ${name} does not match with expected type ${type}`;
}

/**
 * @param operationArguments Arguments coming from the proxy or provider
 * @param parameters List of parameters for the operationArguments
 * @returns Object in the form of { paramDatatypes: [Array of types], params: [Array of values] };
 */
export function transformParameterMapToArray(
    operationArguments: Record<string, any>,
    parameters: { name: string; type: string }[]
): { paramDatatypes: any[]; params: any[] } {
    const params = [];
    const paramDatatypes = [];

    // check if number of parameters in signature matches number of arguments
    if (Object.keys(parameters).length !== Object.keys(operationArguments).length) {
        throw new Error("signature does not match: wrong number of arguments");
    }

    for (let argumentId = 0; argumentId < parameters.length; argumentId++) {
        // check if there's a parameters with the given name
        const argument = parameters[argumentId];
        // retrieve the argument value
        const argumentValue = operationArguments[argument.name];
        // if argument value is not given by the application
        if (UtilInternal.checkNullUndefined(argumentValue)) {
            throw new Error(
                `Cannot call operation with nullable value "${argumentValue}" of argument "${argument.name}"`
            );
        }
        // check if the parameter type matches the type of the argument value
        if (Array.isArray(argumentValue)) {
            if (!argument.type.endsWith("[]")) {
                throw new Error(createErrorMessage("Array", argument.name, argument.type));
            }
        } else {
            const objectType = argumentValue._typeName || argumentValue.constructor.name;

            if (objectType === "Number") {
                if (
                    argument.type !== "Integer" &&
                    argument.type !== "Double" &&
                    argument.type !== "Short" &&
                    argument.type !== "Long" &&
                    argument.type !== "Float" &&
                    argument.type !== "Byte"
                ) {
                    throw new Error(createErrorMessage(objectType, argument.name, argument.type));
                }
            } else if (argument.type !== objectType) {
                throw new Error(createErrorMessage(objectType, argument.name, argument.type));
            }
        }

        paramDatatypes.push(argument.type);
        params.push(argumentValue);
    }
    return {
        paramDatatypes,
        params
    };
}
