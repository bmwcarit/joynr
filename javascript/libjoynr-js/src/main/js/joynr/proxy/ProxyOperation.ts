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
import * as UtilInternal from "../util/UtilInternal";

import * as JSONSerializer from "../util/JSONSerializer";
import * as Typing from "../util/Typing";
import * as MethodUtil from "../util/MethodUtil";
import TypeRegistrySingleton from "../../joynr/types/TypeRegistrySingleton";
import * as Request from "../dispatching/types/Request";
import * as OneWayRequest from "../dispatching/types/OneWayRequest";
import MessagingQos from "../messaging/MessagingQos";
import RequestReplyManager from "../dispatching/RequestReplyManager";

const typeRegistry = TypeRegistrySingleton.getInstance();

function checkArguments(operationArguments: Record<string, any>): void {
    for (const argumentName in operationArguments) {
        if (Object.prototype.hasOwnProperty.call(operationArguments, argumentName)) {
            const argumentValue = operationArguments[argumentName];
            // make sure types of complex type members are also ok
            if (!UtilInternal.checkNullUndefined(argumentValue)) {
                const Constructor = typeRegistry.getConstructor(argumentValue._typeName);

                if (Constructor && Constructor.checkMembers) {
                    Constructor.checkMembers(argumentValue, Typing.checkPropertyAllowObject);
                }
            } else {
                throw new Error(`Argument ${argumentName} is undefined`);
            }
        }
    }
}

function operationFunctionOnSuccess(settings: {
    response: any[];
    settings: { outputParameter: { name: string; type: string }[] };
}): any {
    const response = settings.response;
    const foundValidOperationSignature = settings.settings;
    let argumentValue: Record<string, any> | undefined;
    if (foundValidOperationSignature.outputParameter && foundValidOperationSignature.outputParameter.length > 0) {
        argumentValue = {};
        for (const responseKey in response) {
            if (Object.prototype.hasOwnProperty.call(response, responseKey)) {
                if (foundValidOperationSignature.outputParameter[responseKey] !== undefined) {
                    argumentValue[foundValidOperationSignature.outputParameter[responseKey].name] = Typing.augmentTypes(
                        response[responseKey],
                        foundValidOperationSignature.outputParameter[responseKey].type
                    );
                } else {
                    throw new Error(`Unexpected response: ${JSONSerializer.stringify(response[responseKey])}`);
                }
            }
        }
    }
    return argumentValue;
}

class ProxyOperation {
    private readonly operationSignatures: any[];
    private readonly operationName: string;
    public messagingQos: MessagingQos;
    private requestReplyManager: RequestReplyManager;

    /**
     * The parent proxy object
     */
    protected parent: any;

    /**
     * Constructor of ProxyOperation object that is used in the generation of proxy objects
     *
     * @param parent is the proxy object that contains this attribute
     * @param parent.proxyParticipantId - participantId of the proxy itself
     * @param parent.providerDiscoveryEntry.participantId - participantId of the provider being addressed
     * @param parent.settings.dependencies.requestReplyManager
     * @param operationName the name of the operation
     * @param operationSignatures an array of possible signatures for this operation
     * @param operationSignatures.array.inputParameter an array of supported arguments for
     *            one specific signature
     * @param operationSignatures.array.inputParameter.name the name of the input parameter
     * @param operationSignatures.array.inputParameter.type the type of the input parameter
     * @param operationSignatures.array.outputParameter an array of output parameters for
     *            one specific signature
     * @param operationSignatures.array.outputParameter.name the name of the output parameter
     * @param operationSignatures.array.outputParameter.type the type of the output parameter
     */
    public constructor(
        parent: {
            messagingQos: MessagingQos;
            settings: { dependencies: { requestReplyManager: RequestReplyManager } };
            proxyParticipantId: string;
            providerDiscoveryEntry: { participantId: string };
        },
        operationName: string,
        operationSignatures: {
            inputParameter: { name: string; type: string }[];
            outputParameter?: { name: string; type: string }[];
            fireAndForget?: boolean;
        }[]
    ) {
        this.messagingQos = parent.messagingQos;
        this.requestReplyManager = parent.settings.dependencies.requestReplyManager;
        this.operationName = operationName;
        this.operationSignatures = operationSignatures;
        this.parent = parent;
    }

    /**
     * Generic operation implementation
     *
     * @param operationArguments: this object contains all parameters
     * @param operationArguments.OPERATIONARGUMENTNAME: CUSTOM DOC FROM IDL GOES
     *            HERE, contains the argument value
     *
     * @returns Promise
     */
    public operationFunction(operationArguments: Record<string, any>): Promise<any> {
        try {
            checkArguments(operationArguments);

            let foundValidOperationSignature;
            const caughtErrors = [];

            // cycle through multiple available operation signatures
            for (let i = 0; i < this.operationSignatures.length && foundValidOperationSignature === undefined; ++i) {
                /* transforms input parameters into array, which the api should've been in the first place*/
                try {
                    foundValidOperationSignature = {
                        inputParameter: MethodUtil.transformParameterMapToArray(
                            operationArguments || {},
                            this.operationSignatures[i].inputParameter || {}
                        ),
                        outputParameter: this.operationSignatures[i].outputParameter || [],
                        fireAndForget: this.operationSignatures[i].fireAndForget
                    };
                } catch (e) {
                    caughtErrors.push(e.message);
                }
            }

            // operation was not called because there was no signature found that
            // matches given arguments
            if (foundValidOperationSignature === undefined) {
                return Promise.reject(
                    new Error(
                        `Could not find a valid operation signature in '${JSON.stringify(
                            this.operationSignatures
                        )}' for a call to operation '${this.operationName}' with the arguments: '${JSON.stringify(
                            operationArguments
                        )}'. The following errors occured during signature check: ${JSON.stringify(caughtErrors)}`
                    )
                );
            }

            // send it through request reply manager
            if (foundValidOperationSignature.fireAndForget === true) {
                // build outgoing request
                const oneWayRequest = OneWayRequest.create({
                    methodName: this.operationName,
                    paramDatatypes: foundValidOperationSignature.inputParameter.paramDatatypes,
                    params: foundValidOperationSignature.inputParameter.params
                });

                return this.requestReplyManager.sendOneWayRequest({
                    toDiscoveryEntry: this.parent.providerDiscoveryEntry,
                    from: this.parent.proxyParticipantId,
                    messagingQos: this.messagingQos,
                    request: oneWayRequest
                });
            }

            // build outgoing request
            const request = Request.create({
                methodName: this.operationName,
                paramDatatypes: foundValidOperationSignature.inputParameter.paramDatatypes,
                params: foundValidOperationSignature.inputParameter.params
            });

            return this.requestReplyManager
                .sendRequest(
                    {
                        toDiscoveryEntry: this.parent.providerDiscoveryEntry,
                        from: this.parent.proxyParticipantId,
                        messagingQos: this.messagingQos,
                        request
                    },
                    foundValidOperationSignature
                )
                .then(operationFunctionOnSuccess);
        } catch (e) {
            return Promise.reject(new Error(`error calling operation: ${this.operationName}: ${e}`));
        }
    }

    /**
     * Operation Function builder
     *
     * @returns returns the operation function that can be assigned to a
     *            member of the proxy
     */
    public buildFunction(): Function {
        return this.operationFunction.bind(this);
    }
}

export = ProxyOperation;
