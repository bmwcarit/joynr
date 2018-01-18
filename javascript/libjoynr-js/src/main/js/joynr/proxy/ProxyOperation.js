/*jslint es5: true, node: true */

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
var Promise = require("../../global/Promise");
var Util = require("../util/UtilInternal");
var JSONSerializer = require("../util/JSONSerializer");
var Typing = require("../util/Typing");
var MethodUtil = require("../util/MethodUtil");
var TypeRegistrySingleton = require("../../joynr/types/TypeRegistrySingleton");
var Request = require("../dispatching/types/Request");
var OneWayRequest = require("../dispatching/types/OneWayRequest");
var MessagingQos = require("../messaging/MessagingQos");

var typeRegistry = TypeRegistrySingleton.getInstance();
/**
 * Checks if the given operationSignature is valid to be called for the given operation
 * arguments. valid means, that for all operationArguments there is a matching
 * (name and type) parameter found in the operationSignature and there are no parameters
 * in the operationSignature that miss the corresponding argument.
 *
 * @name ProxyOperation#checkSignatureMatch
 * @function
 * @private
 *
 * @param {Object}
 *            operationSignature an object with the argument name as key and an object
 *            as value defining the type
 * @param {Object}
 *            operationSignature.PARAMETERNAME an object describing the single parameter
 * @param {String}
 *            operationSignature.PARAMETERNAME.type the type of the parameter
 * @param {Object}
 *            operationArguments: this object contains all parameters
 * @param {?}
 *            operationArguments.OPERATIONARGUMENTNAME: CUSTOM DOC FROM IDL GOES HERE,
 *            contains the argument value
 *
 * @returns {Object} if there's a match, a combined object with the name, type and value
 *            is returned, e.g. {"argumentName1":
 *            {type: "string", value: "asdf"},
 *            "argumentName2": {type: "number", value: 1234}}
 *
 * @throws {Error}
 *             if an argument value is nullable
 */
function checkSignatureMatch(operationSignature, operationArguments) {
    // if for all operationArguments there is a matching (name and type) parameter found
    // in the operationSignature, this object will hold name, type and value and is
    // qualified to be used for serialization and will be returned
    var result = {};

    try {
        result.signature = {
            inputParameter: MethodUtil.transformParameterMapToArray(
                operationArguments,
                operationSignature.inputParameter
            ),
            outputParameter: operationSignature.outputParameter || [],
            fireAndForget: operationSignature.fireAndForget
        };
    } catch (error) {
        result.errorMessage = error.message;
    }
    return result;
}

function checkArguments(operationArguments) {
    var errors = [];
    var argumentName;
    var argumentValue;
    for (argumentName in operationArguments) {
        if (operationArguments.hasOwnProperty(argumentName)) {
            argumentValue = operationArguments[argumentName];
            // make sure types of complex type members are also ok
            /*jslint nomen: true */
            if (!Util.checkNullUndefined(argumentValue)) {
                var Constructor = typeRegistry.getConstructor(argumentValue._typeName);

                try {
                    if (Constructor && Constructor.checkMembers) {
                        Constructor.checkMembers(argumentValue, Typing.checkProperty);
                    }
                } catch (error) {
                    errors.push(error.message);
                }
            } else {
                errors.push('Argument "' + argumentName + '" undefined.');
            }
            /*jslint nomen: false */
        }
    }
    return errors;
}

function operationFunctionOnSuccess(settings) {
    var response = settings.response,
        foundValidOperationSignature = settings.settings;
    var responseKey, argumentValue;
    if (foundValidOperationSignature.outputParameter && foundValidOperationSignature.outputParameter.length > 0) {
        argumentValue = {};
        for (responseKey in response) {
            if (response.hasOwnProperty(responseKey)) {
                if (foundValidOperationSignature.outputParameter[responseKey] !== undefined) {
                    argumentValue[foundValidOperationSignature.outputParameter[responseKey].name] = Typing.augmentTypes(
                        response[responseKey],
                        typeRegistry,
                        foundValidOperationSignature.outputParameter[responseKey].type
                    );
                } else {
                    return Promise.reject(
                        new Error("Unexpected response: " + JSONSerializer.stringify(response[responseKey]))
                    );
                }
            }
        }
    }
    return argumentValue;
}

/**
 * Generic operation implementation
 *
 * @name ProxyOperation#operationFunction
 * @function
 * @private
 *
 * @param {Object}
 *            operationArguments: this object contains all parameters
 * @param {?}
 *            operationArguments.OPERATIONARGUMENTNAME: CUSTOM DOC FROM IDL GOES
 *            HERE, contains the argument value
 * @param {DiscoveryQos}
 *            proxyOperation.settings.discoveryQos the Quality of Service parameters
 *            for arbitration
 * @param {MessagingQos}
 *            proxyOperation.settings.messagingQos the Quality of Service parameters
 *            for messaging
 *
 * @param {ProxyOperation}
 *            proxyOperation the ProxyOperation object in which context the
 *            operation is called
 * @param {Array}
 *            operationSignatures a list holding multiple versions of the signature
 *            for the overloaded operation
 * @param {Object}
 *            operationSignatures.array an object with the argument name as key and
 *            an object as value defining the type
 * @param {Object}
 *            operationSignatures.array.PARAMETERNAME an object describing the
 *            single parameter
 * @param {String}
 *            operationSignatures.array.PARAMETERNAME.type the type of the parameter
 *
 * @returns {Object} returns an A+ promise object that will alternatively accept the
 *            callback functions through its
 *            functions "then(function (){..}).catch(function ({string}error){..})"
 *            in A+ promise style instead of using the function parameters
 */
function operationFunction(operationArguments) {
    var i;

    // ensure operationArguments variable holds a valid object and initialize promise object
    var argumentErrors = checkArguments(operationArguments);
    if (argumentErrors.length > 0) {
        return Promise.reject(
            new Error("error calling operation: " + this.operationName + ": " + argumentErrors.toString())
        );
    }

    try {
        var foundValidOperationSignature,
            checkResult,
            caughtErrors = [];

        // cycle through multiple available operation signatures
        for (i = 0; i < this.operationSignatures.length && foundValidOperationSignature === undefined; ++i) {
            // check if the parameters from the operation signature is valid for
            // the provided arguments
            checkResult = checkSignatureMatch(this.operationSignatures[i], operationArguments || {});
            if (checkResult !== undefined) {
                if (checkResult.errorMessage !== undefined) {
                    caughtErrors.push(checkResult.errorMessage);
                } else {
                    foundValidOperationSignature = checkResult.signature;
                }
            }
        }

        // operation was not called because there was no signature found that
        // matches given arguments
        if (foundValidOperationSignature === undefined) {
            return Promise.reject(
                new Error(
                    "Could not find a valid operation signature in '" +
                        JSON.stringify(this.operationSignatures) +
                        "' for a call to operation '" +
                        this.operationName +
                        "' with the arguments: '" +
                        JSON.stringify(operationArguments) +
                        "'. The following errors occured during signature check: " +
                        JSON.stringify(caughtErrors)
                )
            );
        }

        // send it through request reply manager
        if (foundValidOperationSignature.fireAndForget === true) {
            // build outgoing request
            var oneWayRequest = new OneWayRequest({
                methodName: this.operationName,
                paramDatatypes: foundValidOperationSignature.inputParameter.paramDatatypes,
                params: foundValidOperationSignature.inputParameter.params
            });

            return this.settings.dependencies.requestReplyManager.sendOneWayRequest({
                toDiscoveryEntry: this.parent.providerDiscoveryEntry,
                from: this.parent.proxyParticipantId,
                messagingQos: this.messagingQos,
                request: oneWayRequest
            });
        }
        if (foundValidOperationSignature.fireAndForget !== true) {
            // build outgoing request
            var request = new Request({
                methodName: this.operationName,
                paramDatatypes: foundValidOperationSignature.inputParameter.paramDatatypes,
                params: foundValidOperationSignature.inputParameter.params
            });

            return this.settings.dependencies.requestReplyManager
                .sendRequest(
                    {
                        toDiscoveryEntry: this.parent.providerDiscoveryEntry,
                        from: this.parent.proxyParticipantId,
                        messagingQos: this.messagingQos,
                        request: request
                    },
                    foundValidOperationSignature
                )
                .then(operationFunctionOnSuccess);
        }
    } catch (e) {
        return Promise.reject(new Error("error calling operation: " + e.toString()));
    }
}

/**
 * Constructor of ProxyOperation object that is used in the generation of proxy objects
 *
 * @constructor
 * @name ProxyOperation
 *
 * @param {Object}
 *            parent is the proxy object that contains this attribute
 * @param {String}
 *            parent.fromParticipantId of the proxy itself
 * @param {String}
 *            parent.toParticipantId of the provider being addressed
 * @param {Object}
 *            settings the settings object for this function call
 * @param {DiscoveryQos}
 *            settings.discoveryQos the Quality of Service parameters for arbitration
 * @param {MessagingQos}
 *            settings.messagingQos the Quality of Service parameters for messaging
 * @param {Object}
 *            settings.dependencies the dependencies object for this function call
 * @param {RequestReplyManager}
 *            settings.dependencies.requestReplyManager
 * @param {String}
 *            operationName the name of the operation
 * @param {Array}
 *            operationSignatures an array of possible signatures for this operation
 * @param {Array}
 *            operationSignatures.array.inputParameter an array of supported arguments for
 *            one specific signature
 * @param {String}
 *            operationSignatures.array.inputParameter.name the name of the input parameter
 * @param {String}
 *            operationSignatures.array.inputParameter.type the type of the input parameter
 * @param {Array}
 *            operationSignatures.array.outputParameter an array of output parameters for
 *            one specific signature
 * @param {String}
 *            operationSignatures.array.outputParameter.name the name of the output parameter
 * @param {String}
 *            operationSignatures.array.outputParameter.type the type of the output parameter
 */
function ProxyOperation(parent, settings, operationName, operationSignatures) {
    if (!(this instanceof ProxyOperation)) {
        // in case someone calls constructor without new keyword (e.g. var c = Constructor({..}))
        return new ProxyOperation(parent, settings, operationName, operationSignatures);
    }

    // passed in (right-most) messagingQos have precedence; undefined values are
    // ignored
    this.messagingQos = new MessagingQos(Util.extend({}, parent.messagingQos, settings.messagingQos));

    this.settings = settings;

    /**
     * The parent proxy object
     *
     * @name ProxyOperation#parent
     * @type Proxy
     */
    this.parent = parent;

    /**
     * @name ProxyOperation#operationName
     * @type String
     */
    this.operationName = operationName;

    /**
     * @name ProxyOperation#operationSignatures
     * @type Array
     */
    this.operationSignatures = operationSignatures;
}

/**
 * Operation Function builder
 *
 * @name ProxyOperation#buildFunction
 * @function
 *
 * @returns {Function} returns the operation function that can be assigned to a
 *            member of the proxy
 */
ProxyOperation.prototype.buildFunction = function buildFunction() {
    return operationFunction.bind(this);
};

module.exports = ProxyOperation;
