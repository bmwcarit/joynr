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

const ChildProcessUtils = require("./ChildProcessUtils");
ChildProcessUtils.overrideRequirePaths();

const joynr = require("joynr");

const provisioning = require("../../resources/joynr/provisioning/provisioning_cc.js");
const RadioProvider = require("../../generated/joynr/vehicle/RadioProvider.js");
const RadioStation = require("../../generated/joynr/vehicle/radiotypes/RadioStation.js");
const Country = require("../../generated/joynr/datatypes/exampleTypes/Country.js");
const ErrorList = require("../../generated/joynr/vehicle/radiotypes/ErrorList.js");

// attribute value for provider
let isOn = true;
let startWithCapitalLetterValue = true;
let enumAttribute = Country.GERMANY;
let enumArrayAttribute = [Country.GERMANY];
let attrProvidedImpl;
let numberOfStations = -1;
let byteBufferAttribute = null;
let stringMapAttribute = null;
let complexStructMapAttribute = null;
let typeDefForStruct = null;
let typeDefForPrimitive = null;
let mixedSubscriptionsValue = "interval";
let providerDomain;
let libjoynrAsync;

// this delay tries to correct the initialization delay between the web worker and the test driver
const valueChangedInterval = 500;
const mixedSubscriptionDelay = 1500;

let radioProvider;
let providerQos;

function initializeTest(provisioningSuffix, providedDomain) {
    return new Promise((resolve, reject) => {
        // set joynr provisioning
        providerDomain = providedDomain;
        provisioning.persistency = "localStorage";
        provisioning.channelId = `End2EndCommTestParticipantId${provisioningSuffix}`;

        joynr.selectRuntime("inprocess");
        joynr
            .load(provisioning)
            .then(asynclib => {
                libjoynrAsync = asynclib;
                providerQos = new libjoynrAsync.types.ProviderQos({
                    customParameters: [],
                    priority: Date.now(),
                    scope: libjoynrAsync.types.ProviderScope.GLOBAL,
                    supportsOnChangeSubscriptions: true
                });

                // build the provider
                radioProvider = joynr.providerBuilder.build(RadioProvider);

                // register attribute functions
                radioProvider.numberOfStations.registerGetter(() => {
                    return numberOfStations;
                });

                radioProvider.numberOfStations.registerSetter(value => {
                    numberOfStations = value;
                });

                radioProvider.attrProvidedImpl.registerGetter(() => {
                    return attrProvidedImpl;
                });

                radioProvider.attrProvidedImpl.registerSetter(value => {
                    attrProvidedImpl = value;
                });

                radioProvider.mixedSubscriptions.registerGetter(() => {
                    return mixedSubscriptionsValue;
                });

                radioProvider.mixedSubscriptions.registerSetter(() => {});

                radioProvider.attributeTestingProviderInterface.registerGetter(() => {
                    return undefined;
                });

                radioProvider.failingSyncAttribute.registerGetter(() => {
                    throw new joynr.exceptions.ProviderRuntimeException({
                        detailMessage: "failure in failingSyncAttribute getter"
                    });
                });

                radioProvider.failingAsyncAttribute.registerGetter(() => {
                    return new Promise((resolve, reject) => {
                        reject(
                            new joynr.exceptions.ProviderRuntimeException({
                                detailMessage: "failure in failingAsyncAttribute getter"
                            })
                        );
                    });
                });

                radioProvider.StartWithCapitalLetter.registerGetter(() => {
                    return startWithCapitalLetterValue;
                });
                radioProvider.StartWithCapitalLetter.registerSetter(value => {
                    startWithCapitalLetterValue = value;
                });

                radioProvider.isOn.registerGetter(() => {
                    return isOn;
                });
                radioProvider.isOn.registerSetter(value => {
                    isOn = value;
                });

                radioProvider.enumAttribute.registerGetter(() => {
                    return enumAttribute;
                });

                radioProvider.enumAttribute.registerSetter(value => {
                    enumAttribute = value;
                });

                radioProvider.enumArrayAttribute.registerGetter(() => {
                    return enumArrayAttribute;
                });

                radioProvider.enumArrayAttribute.registerSetter(value => {
                    enumArrayAttribute = value;
                });

                radioProvider.byteBufferAttribute.registerSetter(value => {
                    byteBufferAttribute = value;
                });

                radioProvider.byteBufferAttribute.registerGetter(() => {
                    return byteBufferAttribute;
                });

                radioProvider.typeDefForStruct.registerSetter(value => {
                    typeDefForStruct = value;
                });

                radioProvider.typeDefForStruct.registerGetter(() => {
                    return typeDefForStruct;
                });

                radioProvider.typeDefForPrimitive.registerSetter(value => {
                    typeDefForPrimitive = value;
                });

                radioProvider.typeDefForPrimitive.registerGetter(() => {
                    return typeDefForPrimitive;
                });

                radioProvider.stringMapAttribute.registerSetter(value => {
                    stringMapAttribute = value;
                });

                radioProvider.stringMapAttribute.registerGetter(() => {
                    return stringMapAttribute;
                });

                radioProvider.complexStructMapAttribute.registerGetter(() => {
                    return complexStructMapAttribute;
                });

                radioProvider.complexStructMapAttribute.registerSetter(value => {
                    complexStructMapAttribute = value;
                });

                // register operation functions
                radioProvider.addFavoriteStation.registerOperation(opArgs => {
                    // retrieve radioStation name for both overloaded version
                    const name = opArgs.radioStation.name || opArgs.radioStation;

                    // If name contains the string "async" it will work asynchronously
                    // returning a Promise, otherwise synchronously (return/throw directly).
                    // If name contains the string "error" it will throw error or reject
                    // Promise. If name contains "ApplicationException" it will use
                    // the Franca defined error exception for this case, otherwise
                    // ProviderRuntimeException.
                    // If no error handling is active, it will return or resolve true/false
                    // depending on whether radiostation name contains the string "true"
                    if (name.match(/async/)) {
                        // async
                        return new Promise((resolve, reject) => {
                            if (name.match(/error/)) {
                                if (name.match(/ApplicationException/)) {
                                    reject(ErrorList.EXAMPLE_ERROR_1);
                                } else {
                                    reject(
                                        new joynr.exceptions.ProviderRuntimeException({
                                            detailMessage: "example message async"
                                        })
                                    );
                                }
                            } else {
                                resolve({
                                    returnValue: !!name.match(/true/)
                                });
                            }
                        });
                    }
                    // sync
                    if (name.match(/error/)) {
                        if (name.match(/ApplicationException/)) {
                            throw ErrorList.EXAMPLE_ERROR_2;
                        } else {
                            throw new joynr.exceptions.ProviderRuntimeException({
                                detailMessage: "example message sync"
                            });
                        }
                    }
                    return {
                        returnValue: !!name.match(/true/)
                    };
                });

                const isCountryEnum = function(parameter) {
                    return (
                        typeof parameter === "object" &&
                        Object.getPrototypeOf(parameter) instanceof joynr.JoynrObject &&
                        parameter._typeName === Country.GERMANY._typeName
                    );
                };
                const checkEnumInputs = function(opArgs) {
                    let enumElement;
                    if (!isCountryEnum(opArgs.enumInput)) {
                        throw new Error(
                            `Argument enumInput with value ${opArgs.enumInput} is not correctly typed ${
                                Country.GERMANY._typeName
                            }`
                        );
                    }
                    for (enumElement in opArgs.enumArrayInput) {
                        if (opArgs.enumArrayInput.hasOwnProperty(enumElement)) {
                            if (!isCountryEnum(opArgs.enumArrayInput[enumElement])) {
                                throw new Error(
                                    `Argument enumInput with value ${
                                        opArgs.enumArrayInput[enumElement]
                                    } is not correctly typed ${Country.GERMANY._typeName}`
                                );
                            }
                        }
                    }
                };
                // register operation function "operationWithEnumsAsInputAndOutput"
                radioProvider.operationWithEnumsAsInputAndOutput.registerOperation(opArgs => {
                    /* the dummy implemetnation returns the first element of the enumArrayInput.
                     * If the input array is empty, it returns the enumInput
                     */
                    checkEnumInputs(opArgs);
                    let returnValue = opArgs.enumInput;
                    if (opArgs.enumArrayInput.length !== 0) {
                        returnValue = opArgs.enumArrayInput[0];
                    }
                    return {
                        enumOutput: returnValue
                    };
                });

                // register operation function "operationWithMultipleOutputParameters"
                radioProvider.operationWithMultipleOutputParameters.registerOperation(opArgs => {
                    const returnValue = {
                        enumArrayOutput: opArgs.enumArrayInput,
                        enumOutput: opArgs.enumInput,
                        stringOutput: opArgs.stringInput,
                        booleanOutput: opArgs.syncTest
                    };
                    if (opArgs.syncTest) {
                        return returnValue;
                    }
                    return Promise.resolve(returnValue);
                });

                // register operation function "operationWithEnumsAsInputAndEnumArrayAsOutput"
                radioProvider.operationWithEnumsAsInputAndEnumArrayAsOutput.registerOperation(opArgs => {
                    /* the dummy implementation returns the enumArrayInput.
                     * If the enumInput is not empty, it add this entry to the return value as well
                     */
                    checkEnumInputs(opArgs);
                    const returnValue = opArgs.enumArrayInput;
                    if (opArgs.enumInput !== undefined) {
                        returnValue.push(opArgs.enumInput);
                    }
                    return {
                        enumOutput: returnValue
                    };
                });

                // register operation function "methodWithSingleArrayParameters"
                radioProvider.methodWithSingleArrayParameters.registerOperation(opArgs => {
                    /* the dummy implementation transforms the incoming double values into
                     * strings.
                     */
                    const stringArrayOut = [];
                    if (opArgs.doubleArrayArg !== undefined) {
                        for (const element in opArgs.doubleArrayArg) {
                            if (opArgs.doubleArrayArg.hasOwnProperty(element)) {
                                stringArrayOut.push(opArgs.doubleArrayArg[element].toString());
                            }
                        }
                    }
                    return {
                        stringArrayOut
                    };
                });

                // register operation function "methodWithByteBuffer"
                radioProvider.methodWithByteBuffer.registerOperation(opArgs => {
                    /* the dummy implementation returns the incoming byteBuffer
                     */

                    return {
                        result: opArgs.input
                    };
                });

                // register operation function "methodWithTypeDef"
                radioProvider.methodWithTypeDef.registerOperation(opArgs => {
                    /* the dummy implementation returns the incoming data
                     */

                    return {
                        typeDefStructOutput: opArgs.typeDefStructInput,
                        typeDefPrimitiveOutput: opArgs.typeDefPrimitiveInput
                    };
                });

                radioProvider.methodWithComplexMap.registerOperation(() => {
                    return;
                });

                radioProvider.methodProvidedImpl.registerOperation(opArgs => {
                    return {
                        returnValue: opArgs.arg
                    };
                });

                function triggerBroadcastsInternal(opArgs) {
                    let outputParams, broadcast;
                    if (opArgs.broadcastName === "broadcastWithEnum") {
                        //broadcastWithEnum
                        broadcast = radioProvider.broadcastWithEnum;
                        outputParams = broadcast.createBroadcastOutputParameters();
                        outputParams.setEnumOutput(Country.CANADA);
                        outputParams.setEnumArrayOutput([Country.GERMANY, Country.ITALY]);
                    } else if (opArgs.broadcastName === "emptyBroadcast") {
                        broadcast = radioProvider.emptyBroadcast;
                        outputParams = broadcast.createBroadcastOutputParameters();
                    } else if (opArgs.broadcastName === "weakSignal") {
                        //weakSignal
                        broadcast = radioProvider.weakSignal;
                        outputParams = broadcast.createBroadcastOutputParameters();
                        outputParams.setRadioStation("radioStation");
                        outputParams.setByteBuffer([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0]);
                    } else if (opArgs.broadcastName === "broadcastWithTypeDefs") {
                        //broadcastWithTypeDefs
                        broadcast = radioProvider.broadcastWithTypeDefs;
                        outputParams = broadcast.createBroadcastOutputParameters();
                        outputParams.setTypeDefStructOutput(
                            new RadioStation({
                                name: "TestEnd2EndCommProviderProcess.broadcastWithTypeDefs.RadioStation",
                                byteBuffer: []
                            })
                        );
                        outputParams.setTypeDefPrimitiveOutput(123456);
                    }
                    setTimeout(
                        (opArgs, broadcast, outputParams) => {
                            let i;
                            for (i = 0; i < opArgs.times; i++) {
                                if (opArgs.hierarchicBroadcast && opArgs.partitions !== undefined) {
                                    const hierarchicPartitions = [];
                                    broadcast.fire(outputParams, hierarchicPartitions);
                                    for (let j = 0; j < opArgs.partitions.length; j++) {
                                        hierarchicPartitions.push(opArgs.partitions[j]);
                                        broadcast.fire(outputParams, hierarchicPartitions);
                                    }
                                } else {
                                    broadcast.fire(outputParams, opArgs.partitions);
                                }
                            }
                        },
                        0,
                        opArgs,
                        broadcast,
                        outputParams
                    );
                }

                radioProvider.triggerBroadcasts.registerOperation(triggerBroadcastsInternal);
                radioProvider.triggerBroadcastsWithPartitions.registerOperation(triggerBroadcastsInternal);

                radioProvider.methodFireAndForgetWithoutParams.registerOperation(() => {
                    const broadcast = radioProvider.fireAndForgetCallArrived;
                    const outputParams = broadcast.createBroadcastOutputParameters();
                    outputParams.setMethodName("methodFireAndForgetWithoutParams");
                    broadcast.fire(outputParams);
                });

                radioProvider.methodFireAndForget.registerOperation(() => {
                    const broadcast = radioProvider.fireAndForgetCallArrived;
                    const outputParams = broadcast.createBroadcastOutputParameters();
                    outputParams.setMethodName("methodFireAndForget");
                    broadcast.fire(outputParams);
                });

                providerQos.priority = Date.now();
                // register provider at the given providerDomain
                libjoynrAsync.registration
                    .registerProvider(providerDomain, radioProvider, providerQos)
                    .then(() => {
                        // signal test driver that we are ready
                        resolve(libjoynrAsync);
                    })
                    .catch(error => {
                        reject(error);
                        throw new Error(`error registering provider: ${error}`);
                    });

                return libjoynrAsync;
            })
            .catch(error => {
                throw error;
            });
    });
}

function startTest() {
    // change attribute value of numberOfStations periodically
    libjoynrAsync.util.LongTimer.setInterval(() => {
        radioProvider.numberOfStations.valueChanged(++numberOfStations);
    }, valueChangedInterval);

    libjoynrAsync.util.LongTimer.setTimeout(() => {
        mixedSubscriptionsValue = "valueChanged1";
        radioProvider.mixedSubscriptions.valueChanged(mixedSubscriptionsValue);
        libjoynrAsync.util.LongTimer.setTimeout(() => {
            mixedSubscriptionsValue = "valueChanged2";
            radioProvider.mixedSubscriptions.valueChanged(mixedSubscriptionsValue);
        }, 10);
    }, mixedSubscriptionDelay);
    return Promise.resolve(libjoynrAsync.participantIdStorage.getParticipantId(providerDomain, radioProvider));
}
function terminateTest() {
    return libjoynrAsync.registration.unregisterProvider(providerDomain, radioProvider);
}

ChildProcessUtils.registerHandlers(initializeTest, startTest, terminateTest);
