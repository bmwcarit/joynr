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
import * as Typing from "../util/Typing";
import * as MethodUtil from "../util/MethodUtil";
import ApplicationException from "../exceptions/ApplicationException";
import ProviderRuntimeException from "../exceptions/ProviderRuntimeException";
/**
 * Checks if the given argumentDatatypes and arguments match the given operationSignature
 *
 * @param unnamedArguments an array containing the arguments, e.g. [1234, "asdf"]
 * @param argumentDatatypes an array containing the datatypes, e.g. ["Integer", "String"]
 * @param operationSignature operationSignature of called method
 *
 * @returns undefined if argumentDatatypes does not match operationSignature or a map
 *            containing a named argument map, e.g. &#123;nr: 1234,str: "asdf"&#125;
 */
function getNamedArguments(
    unnamedArguments: any[],
    argumentDatatypes: string[],
    operationSignature: OperationSignature
): Record<string, any> | void {
    let argument, argumentName;
    const namedArguments: Record<string, any> = {};
    const inputParameter = operationSignature.inputParameter;

    // check if number of given argument types (argumentDatatypes.length) matches number
    // of parameters in op signature (keys.length)
    if (argumentDatatypes.length !== inputParameter.length) {
        return undefined;
    }

    // cycle over all arguments
    for (let i = 0; i < inputParameter.length; ++i) {
        argument = inputParameter[i];
        argumentName = argument.name;
        // check if argument type matches parameter's type from operation signature
        if (argumentDatatypes[i] !== argument.type) {
            return undefined;
        }

        // put argument value into named arguments map
        namedArguments[argumentName] = unnamedArguments[i];
    }

    return namedArguments;
}

function returnValueToResponseArray(
    returnValue: any,
    outputParameter: {
        name: string;
        type: string;
    }[]
): any[] {
    if (outputParameter.length === 0) {
        return [];
    }
    /*
     * In case of multiple output parameters, we expect that the provider returns a key-value-pair
     */
    return MethodUtil.transformParameterMapToArray(returnValue || {}, outputParameter).params;
}

interface OperationSignature {
    inputParameter: {
        name: string;
        type: string;
    }[];
    error: {
        type: string;
    };
    outputParameter: {
        name: string;
        type: string;
    }[];
}

class ProviderOperation {
    private operationSignatures: OperationSignature[];
    public operationName: string;
    private privateOperationFunc: Function;
    /**
     * Constructor of ProviderAttribute object that is used in the generation of provider objects
     * @param [implementation] the operation function
     * @param operationName the name of the operation
     * @param operationSignatures an object with the argument name as key and an object
     *            as value defining the type
     * @param operationSignatures.array an object with the argument name as key and an
     *            object as value defining the type
     * @param operationSignatures.array.PARAMETERNAME an object describing the single
     *            parameter
     * @param operationSignatures.array.PARAMETERNAME.type the type of the parameter
     */
    public constructor(implementation: Function, operationName: string, operationSignatures: OperationSignature[]) {
        this.privateOperationFunc = implementation;
        this.operationName = operationName;
        this.operationSignatures = operationSignatures;
    }

    /**
     * Registers the operation function
     *
     * @param operationFunc registers the operation function
     */
    public registerOperation(operationFunc: Function): void {
        this.privateOperationFunc = operationFunc;
    }

    /**
     * Calls the operation function.
     *
     * @param operationArguments the operation arguments as an array
     * @param operationArguments the operation argument value, e.g. 1
     * @param operationArgumentTypes the operation argument types as an array
     * @param operationArgumentTypes the operation argument type in String form e.g. "Integer"
     *
     * @returns the return type of the called operation function
     */
    public async callOperation(operationArguments: any, operationArgumentTypes: string[]): Promise<any> {
        let argument, namedArguments, signature!: OperationSignature;

        // cycle through multiple available operation signatures
        for (let i = 0; i < this.operationSignatures.length && namedArguments === undefined; ++i) {
            signature = this.operationSignatures[i];
            // check if the parameters from the operation signature is valid for
            // the provided arguments
            namedArguments = getNamedArguments(operationArguments, operationArgumentTypes, signature);
        }

        if (namedArguments) {
            // augment types
            for (let j = 0; j < signature.inputParameter.length; ++j) {
                argument = signature.inputParameter[j];
                namedArguments[argument.name] = Typing.augmentTypes(namedArguments[argument.name], argument.type);
            }

            try {
                const returnValue = await this.privateOperationFunc(namedArguments);
                return returnValueToResponseArray(returnValue, signature.outputParameter || []);
            } catch (exceptionOrErrorEnumValue) {
                let exception;
                if (exceptionOrErrorEnumValue instanceof ProviderRuntimeException) {
                    exception = exceptionOrErrorEnumValue;
                } else if (Typing.isComplexJoynrObject(exceptionOrErrorEnumValue)) {
                    exception = new ApplicationException({
                        detailMessage: "Application exception, details see error enum",
                        error: exceptionOrErrorEnumValue
                    });
                } else if (exceptionOrErrorEnumValue instanceof Error) {
                    exception = new ProviderRuntimeException({
                        detailMessage: `Implementation causes unknown error: ${exceptionOrErrorEnumValue.message}`
                    });
                } else {
                    exception = new ProviderRuntimeException({
                        detailMessage: "Implementation causes unknown error"
                    });
                }
                throw exception;
            }
        }

        throw new Error(
            `Could not find a valid operation signature in '${JSON.stringify(
                this.operationSignatures
            )}' for a call to operation '${this.operationName}' with the arguments: '${JSON.stringify(
                operationArguments
            )}'`
        );
    }

    /**
     * Check if the registered operation is defined.
     * @returns if the registered operation is defined.
     */
    public checkOperation(): boolean {
        return typeof this.privateOperationFunc === "function";
    }
}

export = ProviderOperation;
