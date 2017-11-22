/*jslint es5: true, nomen: true */
/*global Promise: true, WorkerUtils: true, importScripts: true, joynr: true, Country: true, RadioStation: true, RadioProvider: true, domain: true, interfaceNameComm: true, providerParticipantIdComm: true, providerChannelIdComm: true, globalCapDirCapability: true, channelUrlDirCapability: true, ErrorList: true */

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

// anything that you load here is served through the jsTestDriverServer, if you add an entry you
// have to make it available through the jsTestDriverIntegrationTests.conf

importScripts("WorkerUtils.js");
importScripts("../joynr/provisioning/provisioning_root.js");
importScripts("LocalStorageSimulator.js");

importScripts("../../jar-classes/joynr.js");
importScripts("../joynr/provisioning/provisioning_cc.js");

var document = { URL: window.joynr.provisioning.brokerUri };

importScripts("provisioning_end2end_common.js");
importScripts("../joynr/vehicle/RadioProvider.js");
importScripts("../joynr/vehicle/radiotypes/RadioStation.js");
importScripts("../joynr/tests/testTypes/ComplexTestType.js");
importScripts("../joynr/datatypes/exampleTypes/Country.js");
importScripts("../joynr/datatypes/exampleTypes/StringMap.js");
importScripts("../joynr/datatypes/exampleTypes/ComplexStructMap.js");
importScripts("../joynr/datatypes/exampleTypes/ComplexStruct.js");
importScripts("../joynr/vehicle/radiotypes/ErrorList.js");
importScripts("../../classes/lib/bluebird.js");

var Promise = Promise.Promise;
// attribute value for provider
var isOn = true;
var startWithCapitalLetterValue = true;
var enumAttribute = Country.GERMANY;
var enumArrayAttribute = [Country.GERMANY];
var attrProvidedImpl;
var numberOfStations = -1;
var mixedSubscriptions = null;
var byteBufferAttribute = null;
var stringMapAttribute = null;
var complexStructMapAttribute = null;
var typeDefForStruct = null;
var typeDefForPrimitive = null;
var mixedSubscriptionsValue = "interval";
var providerDomain;
var libjoynrAsync;

// this delay tries to correct the initialization delay between the web worker and the test driver
var valueChangedInterval = 500;
var mixedSubscriptionDelay = 1500;

var radioProvider;

var providerQos;

function initializeTest(provisioningSuffix, providedDomain) {
    return new Promise(function(resolve, reject) {

        // set joynr provisioning
        providerDomain = (providedDomain !== undefined ? providedDomain : domain);
        localStorage.setItem(
        "joynr.participants." + providerDomain + interfaceNameComm,
        providerParticipantIdComm + provisioningSuffix);
        joynr.provisioning.persistency = "localStorage";

        joynr.provisioning.channelId = providerChannelIdComm + provisioningSuffix;
        joynr.provisioning.logging = {
            configuration : {
                name : "TestEnd2EndCommProviderWorkLogging",
                appenders : {
                    WebWorker : {
                        name : "WEBWORKER"
                    }
                },
                loggers : {
                    root : {
                        level : "debug",
                        AppenderRef : {
                            ref : "WEBWORKER"
                        }
                    }
                }
            }
        };

        joynr.load(joynr.provisioning).then(function(asynclib){
            libjoynrAsync = asynclib;
            providerQos = new libjoynrAsync.types.ProviderQos({
                customParameters : [],
                priority : Date.now(),
                scope : libjoynrAsync.types.ProviderScope.GLOBAL,
                supportsOnChangeSubscriptions : true
            });

            // build the provider
            radioProvider = joynr.providerBuilder.build(RadioProvider);

            // register attribute functions
            radioProvider.numberOfStations.registerGetter(function() {
                return numberOfStations;
            });

            radioProvider.numberOfStations.registerSetter(function(value) {
                numberOfStations = value;
            });

            radioProvider.attrProvidedImpl.registerGetter(function() {
                return attrProvidedImpl;
            });

            radioProvider.attrProvidedImpl.registerSetter(function(value) {
                attrProvidedImpl = value;
            });

            radioProvider.mixedSubscriptions.registerGetter(function() {
                return mixedSubscriptionsValue;
            });

            radioProvider.mixedSubscriptions.registerSetter(function(value) {
                mixedSubscriptions = value;
            });

            radioProvider.attributeTestingProviderInterface.registerGetter(function() {
                return undefined;
            });

            radioProvider.failingSyncAttribute.registerGetter(function() {
                throw new joynr.exceptions.ProviderRuntimeException({
                    detailMessage: "failure in failingSyncAttribute getter"
                });
            });

            radioProvider.failingAsyncAttribute.registerGetter(function() {
                return new Promise(function(resolve, reject) {
                    reject(new joynr.exceptions.ProviderRuntimeException({
                        detailMessage: "failure in failingAsyncAttribute getter"
                    }));
                });
            });

            radioProvider.StartWithCapitalLetter.registerGetter(function() {
                return startWithCapitalLetterValue;
            });
            radioProvider.StartWithCapitalLetter.registerSetter(function(value) {
                startWithCapitalLetterValue = value;
            });

            radioProvider.isOn.registerGetter(function() {
                return isOn;
            });
            radioProvider.isOn.registerSetter(function(value) {
                isOn = value;
            });

            radioProvider.enumAttribute.registerGetter(function() {
                return enumAttribute;
            });

            radioProvider.enumAttribute.registerSetter(function(value) {
                enumAttribute = value;
            });

            radioProvider.enumArrayAttribute.registerGetter(function() {
                return enumArrayAttribute;
            });

            radioProvider.enumArrayAttribute.registerSetter(function(value) {
                enumArrayAttribute = value;
            });

            radioProvider.byteBufferAttribute.registerSetter(function(value) {
                byteBufferAttribute = value;
            });

            radioProvider.byteBufferAttribute.registerGetter(function(value) {
                return byteBufferAttribute;
            });

            radioProvider.typeDefForStruct.registerSetter(function(value) {
                typeDefForStruct = value;
            });

            radioProvider.typeDefForStruct.registerGetter(function(value) {
                return typeDefForStruct;
            });

            radioProvider.typeDefForPrimitive.registerSetter(function(value) {
                typeDefForPrimitive = value;
            });

            radioProvider.typeDefForPrimitive.registerGetter(function(value) {
                return typeDefForPrimitive;
            });

            radioProvider.stringMapAttribute.registerSetter(function(value) {
                stringMapAttribute = value;
            });

            radioProvider.stringMapAttribute.registerGetter(function() {
                return stringMapAttribute;
            });

            radioProvider.complexStructMapAttribute.registerGetter(function() {
                return complexStructMapAttribute;
            });

            radioProvider.complexStructMapAttribute.registerSetter(function(value) {
                complexStructMapAttribute = value;
            });

            // register operation functions
            radioProvider.addFavoriteStation.registerOperation(function(opArgs) {
                // retrieve radioStation name for both overloaded version
                var name = opArgs.radioStation.name || opArgs.radioStation;

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
                    return new Promise(function(resolve, reject) {
                        if (name.match(/error/)) {
                            if (name.match(/ApplicationException/)) {
                                reject(ErrorList.EXAMPLE_ERROR_1);
                            } else {
                                reject(new joynr.exceptions.ProviderRuntimeException({ detailMessage: "example message async" }));
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
                        throw new joynr.exceptions.ProviderRuntimeException({ detailMessage: "example message sync" });
                    }
                }
                return {
                    returnValue: !!name.match(/true/)
                };
            });

            var isCountryEnum = function(parameter) {
                return typeof parameter === "object" &&
                Object.getPrototypeOf(parameter) instanceof joynr.JoynrObject &&
                parameter._typeName === Country.GERMANY._typeName;
            };
            var checkEnumInputs = function(opArgs) {
                var enumElement;
                if (!isCountryEnum(opArgs.enumInput)) {
                    throw new Error("Argument enumInput with value " + opArgs.enumInput + " is not correctly typed " + Country.GERMANY._typeName);
                }
                for (enumElement in opArgs.enumArrayInput) {
                    if (opArgs.enumArrayInput.hasOwnProperty(enumElement)) {
                        if (!isCountryEnum(opArgs.enumArrayInput[enumElement])) {
                            throw new Error("Argument enumInput with value " + opArgs.enumArrayInput[enumElement] + " is not correctly typed " + Country.GERMANY._typeName);
                        }
                    }
                }
            };
            // register operation function "operationWithEnumsAsInputAndOutput"
            radioProvider.operationWithEnumsAsInputAndOutput.registerOperation(function(opArgs) {
                /* the dummy implemetnation returns the first element of the enumArrayInput.
                 * If the input array is empty, it returns the enumInput
                 */
                checkEnumInputs(opArgs);
                var returnValue = opArgs.enumInput;
                if (opArgs.enumArrayInput.length !== 0) {
                    returnValue = opArgs.enumArrayInput[0];
                }
                return {
                    enumOutput: returnValue
                };
            });

            // register operation function "operationWithMultipleOutputParameters"
            radioProvider.operationWithMultipleOutputParameters.registerOperation(function(opArgs) {
                var returnValue = {
                    enumArrayOutput: opArgs.enumArrayInput,
                    enumOutput: opArgs.enumInput,
                    stringOutput: opArgs.stringInput,
                    booleanOutput: opArgs.syncTest,
                };
                if (opArgs.syncTest) {
                    return returnValue;
                }
                return new Promise(function(resolve, reject){
                    resolve(returnValue);
                });
            });

            // register operation function "operationWithEnumsAsInputAndEnumArrayAsOutput"
            radioProvider.operationWithEnumsAsInputAndEnumArrayAsOutput.registerOperation(function(opArgs) {
                /* the dummy implementation returns the enumArrayInput.
                 * If the enumInput is not empty, it add this entry to the return value as well
                 */
                checkEnumInputs(opArgs);
                var returnValue = opArgs.enumArrayInput;
                if (opArgs.enumInput !== undefined) {
                    returnValue.push(opArgs.enumInput);
                }
                return {
                    enumOutput: returnValue
                };
            });

            // register operation function "methodWithSingleArrayParameters"
            radioProvider.methodWithSingleArrayParameters.registerOperation(function(opArgs) {
                /* the dummy implementation transforms the incoming double values into
                 * strings.
                 */
                var stringArrayOut = [], element;
                if (opArgs.doubleArrayArg !== undefined) {
                    for (element in opArgs.doubleArrayArg) {
                        if (opArgs.doubleArrayArg.hasOwnProperty(element)) {
                            stringArrayOut.push(opArgs.doubleArrayArg[element].toString());
                        }
                    }
                }
                return {
                    stringArrayOut: stringArrayOut
                };
            });

            // register operation function "methodWithByteBuffer"
            radioProvider.methodWithByteBuffer.registerOperation(function(opArgs) {
                /* the dummy implementation returns the incoming byteBuffer
                 */

                return {
                    result: opArgs.input
                };
            });

            // register operation function "methodWithTypeDef"
            radioProvider.methodWithTypeDef.registerOperation(function(opArgs) {
                /* the dummy implementation returns the incoming data
                 */

                return {
                    typeDefStructOutput: opArgs.typeDefStructInput,
                    typeDefPrimitiveOutput: opArgs.typeDefPrimitiveInput
                };
            });

            radioProvider.methodWithComplexMap.registerOperation(function(
            opArgs) {
                return;
            });

            radioProvider.methodProvidedImpl.registerOperation(function(opArgs) {
                return {
                    returnValue : opArgs.arg
                };
            });

            function triggerBroadcastsInternal(opArgs) {
                var outputParams, broadcast;
                if (opArgs.broadcastName === "broadcastWithEnum") {
                    //broadcastWithEnum
                    broadcast = radioProvider.broadcastWithEnum;
                    outputParams = broadcast.createBroadcastOutputParameters();
                    outputParams.setEnumOutput(Country.CANADA);
                    outputParams.setEnumArrayOutput([Country.GERMANY, Country.ITALY]);
                } else if (opArgs.broadcastName === "emptyBroadcast"){
                    broadcast = radioProvider.emptyBroadcast;
                    outputParams = broadcast.createBroadcastOutputParameters();
                } else if (opArgs.broadcastName === "weakSignal"){
                    //weakSignal
                    broadcast = radioProvider.weakSignal;
                    outputParams = broadcast.createBroadcastOutputParameters();
                    outputParams.setRadioStation("radioStation");
                    outputParams.setByteBuffer([0,1,2,3,4,5,6,7,8,9,8,7,6,5,4,3,2,1,0]);
                } else if (opArgs.broadcastName === "broadcastWithTypeDefs"){
                    //broadcastWithTypeDefs
                    broadcast = radioProvider.broadcastWithTypeDefs;
                    outputParams = broadcast.createBroadcastOutputParameters();
                    outputParams.setTypeDefStructOutput(new RadioStation({
                        name: "TestEnd2EndCommProviderWorker.broadcastWithTypeDefs.RadioStation",
                        byteBuffer: []
                    }));
                    outputParams.setTypeDefPrimitiveOutput(123456);
                }
                setTimeout(function(opArgs, broadcast, outputParams) {
                    var i;
                    for (i = 0; i < opArgs.times; i++) {
                        if (opArgs.hierarchicBroadcast && opArgs.partitions !== undefined) {
                            var j, hierarchicPartitions = [];
                            broadcast.fire(outputParams, hierarchicPartitions);
                            for (j=0;j<opArgs.partitions.length;j++) {
                                hierarchicPartitions.push(opArgs.partitions[j]);
                                broadcast.fire(outputParams, hierarchicPartitions);
                            }
                        } else {
                            broadcast.fire(outputParams, opArgs.partitions);
                        }
                    }
                },0, opArgs, broadcast, outputParams);
            }

            radioProvider.triggerBroadcasts.registerOperation(triggerBroadcastsInternal);
            radioProvider.triggerBroadcastsWithPartitions.registerOperation(triggerBroadcastsInternal);

            radioProvider.methodFireAndForgetWithoutParams.registerOperation(function(opArgs) {
                var broadcast = radioProvider.fireAndForgetCallArrived;
                var outputParams = broadcast.createBroadcastOutputParameters();
                outputParams.setMethodName("methodFireAndForgetWithoutParams");
                broadcast.fire(outputParams);
            });

            radioProvider.methodFireAndForget.registerOperation(function(opArgs) {
                var broadcast = radioProvider.fireAndForgetCallArrived;
                var outputParams = broadcast.createBroadcastOutputParameters();
                outputParams.setMethodName("methodFireAndForget");
                broadcast.fire(outputParams);
            });

            providerQos.priority = Date.now();
            // register provider at the given providerDomain
            libjoynrAsync.registration.registerProvider(
            providerDomain,
            radioProvider,
            providerQos).then(function() {
                // signal test driver that we are ready
                resolve(libjoynrAsync);
                return;
            }).catch(function(error) {
                reject(error);
                throw new Error("error registering provider: " + error);
            });

            return libjoynrAsync;
        }).catch(function(error){
            throw error;
        });
    });
}

function startTest() {
    return new Promise(function(resolve, reject) {
        // change attribute value of numberOfStations periodically
        libjoynrAsync.util.LongTimer.setInterval(function() {
            radioProvider.numberOfStations.valueChanged(++numberOfStations);
        }, valueChangedInterval);

        libjoynrAsync.util.LongTimer.setTimeout(function() {
            mixedSubscriptionsValue = "valueChanged1";
            radioProvider.mixedSubscriptions.valueChanged(mixedSubscriptionsValue);
            libjoynrAsync.util.LongTimer.setTimeout(function() {
                mixedSubscriptionsValue = "valueChanged2";
                radioProvider.mixedSubscriptions.valueChanged(mixedSubscriptionsValue);
            }, 10);
        }, mixedSubscriptionDelay);
        resolve(libjoynrAsync.participantIdStorage.getParticipantId(
        providerDomain,
        radioProvider));
    });
}
function terminateTest() {
    return libjoynrAsync.registration.unregisterProvider(providerDomain, radioProvider);
}
