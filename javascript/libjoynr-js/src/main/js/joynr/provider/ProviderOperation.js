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
const MethodUtil = require("../util/MethodUtil");
const TypeRegistrySingleton = require("../../joynr/types/TypeRegistrySingleton");
const ApplicationException = require("../exceptions/ApplicationException");
const ProviderRuntimeException = require("../exceptions/ProviderRuntimeException");
const Promise = require("../../global/Promise");

const typeRegistry = TypeRegistrySingleton.getInstance();
/**
 * Checks if the given argumentDatatypes and arguments match the given operationSignature
 *
 * @name ProviderOperation#getNamedArguments
 * @function
 * @private
 *
 * @param {Array}
 *            unnamedArguments an array containing the arguments, e.g. [1234, "asdf"]
 * @param {?}
 *            unnamedArguments.array the argument value
 * @param {Array}
 *            argumentDatatypes an array containing the datatypes,
 *            e.g. ["Integer", "String"]
 * @param {String}
 *            argumentDatatypes.array the datatype in string format
 * @param {Array}
 *            operationSignatures an array of possible signatures for this operation
 * @param {Array}
 *            operationSignatures.array.inputParameter an array of supported arguments for
 *            one specific signature
 * @param {String}
 *            operationSignatures.array.inputParameter.name the name of the input parameter
 * @param {String}
 *            operationSignatures.array.inputParameter.type the type of the input parameter
 *
 * @returns undefined if argumentDatatypes does not match operationSignature or a map
 *            containing a named argument map, e.g. &#123;nr: 1234,str: "asdf"&#125;
 */
function getNamedArguments(unnamedArguments, argumentDatatypes, operationSignature) {
    let argument, argumentName;
    const namedArguments = {};
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

function returnValueToResponseArray(returnValue, outputParameter) {
    if (outputParameter.length === 0) {
        return [];
    }
    /*
     * In case of multiple output parameters, we expect that the provider returns a key-value-pair
     */
    return MethodUtil.transformParameterMapToArray(returnValue || {}, outputParameter).params;
}
/**
 * Constructor of ProviderAttribute object that is used in the generation of provider
 * objects
 *
 * @name ProviderOperation
 * @constructor
 *
 * @param {Provider}
 *            parent the provider object
 *
 * @param {Object}
 *            [implementation] the operation function
 *
 * @param {String}
 *            operationName the name of the operation
 *
 * @param {Array}
 *            operationSignatures an object with the argument name as key and an object
 *            as value defining the type
 * @param {Object}
 *            operationSignatures.array an object with the argument name as key and an
 *            object as value defining the type
 * @param {Object}
 *            operationSignatures.array.PARAMETERNAME an object describing the single
 *            parameter
 * @param {String}
 *            operationSignatures.array.PARAMETERNAME.type the type of the parameter
 */
function ProviderOperation(parent, implementation, operationName, operationSignatures) {
    if (!(this instanceof ProviderOperation)) {
        // in case someone calls constructor without new keyword
        // (e.g. var c = Constructor({..}))
        return new ProviderOperation(parent, implementation, operationName, operationSignatures);
    }

    this._privateOperationFunc = implementation;
    this._operationName = operationName;
    this._operationSignatures = operationSignatures;
}

/**
 * Registers the operation function
 *
 * @name ProviderOperation#registerOperation
 * @function
 *
 * @param {Function}
 *            operationFunc registers the operation function
 */
ProviderOperation.prototype.registerOperation = function registerOperation(operationFunc) {
    this._privateOperationFunc = operationFunc;
};

function privateOperationOnError(exceptionOrErrorEnumValue) {
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
            detailMessage: "Implementation causes unknown error: " + exceptionOrErrorEnumValue.message
        });
    } else {
        exception = new ProviderRuntimeException({
            detailMessage: "Implementation causes unknown error"
        });
    }
    throw exception;
}

/**
 * Calls the operation function.
 *
 * @name ProviderOperation#callOperation
 * @function
 *
 * @param {Array}
 *            operationArguments the operation arguments as an array
 * @param {?}
 *            operationArguments the operation argument value, e.g. 1
 * @param {Array}
 *            operationArgumentTypes the operation argument types as an array
 * @param {String}
 *            operationArgumentTypes the operation argument type in String form
 *            e.g. "Integer"
 *
 * @returns {?} the return type of the called operation function
 */
ProviderOperation.prototype.callOperation = function callOperation(operationArguments, operationArgumentTypes) {
    let i, j;
    let argument, namedArguments, signature;

    // cycle through multiple available operation signatures
    for (i = 0; i < this._operationSignatures.length && namedArguments === undefined; ++i) {
        signature = this._operationSignatures[i];
        // check if the parameters from the operation signature is valid for
        // the provided arguments
        namedArguments = getNamedArguments(operationArguments, operationArgumentTypes, signature);
    }

    function privateOperationOnSuccess(returnValue) {
        return returnValueToResponseArray(returnValue, signature.outputParameter || []);
    }

    if (namedArguments) {
        // augment types
        for (j = 0; j < signature.inputParameter.length; ++j) {
            argument = signature.inputParameter[j];
            namedArguments[argument.name] = Typing.augmentTypes(
                namedArguments[argument.name],
                typeRegistry,
                argument.type
            );
        }

        // By starting a promise chain, privateOperationFunc will be converted into a promise as well
        return Promise.resolve(namedArguments)
            .then(this._privateOperationFunc)
            .then(privateOperationOnSuccess)
            .catch(privateOperationOnError);
    }

    // TODO: proper error handling
    throw new Error(
        "Could not find a valid operation signature in '" +
            JSON.stringify(this._operationSignatures) +
            "' for a call to operation '" +
            this._operationName +
            "' with the arguments: '" +
            JSON.stringify(operationArguments) +
            "'"
    );
};

/**
 * Check if the registered operation is defined.
 * @function ProviderOperation#checkOperation
 * @returns {Boolean}
 */
ProviderOperation.prototype.checkOperation = function checkOperation() {
    return typeof this._privateOperationFunc === "function";
};

module.exports = ProviderOperation;
