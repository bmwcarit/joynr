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

import * as DatatypeForTestLibjoynr from "../../../generated/joynr/vehicle/radiotypes/DatatypeForTestLibjoynr";
import * as ChildProcessUtils from "../ChildProcessUtils";

import joynr from "joynr";
import provisioning from "../../../resources/joynr/provisioning/provisioning_cc";
import RadioProvider from "../../../generated/joynr/vehicle/RadioProvider";
import RadioStation from "../../../generated/joynr/vehicle/radiotypes/RadioStation";
import Country from "../../../generated/joynr/datatypes/exampleTypes/Country";
import ErrorList from "../../../generated/joynr/vehicle/radiotypes/ErrorList";
import StringMap = require("../../../generated/joynr/datatypes/exampleTypes/StringMap");
import ComplexStructMap = require("../../../generated/joynr/datatypes/exampleTypes/ComplexStructMap");
import InProcessRuntime = require("../../../../main/js/joynr/start/InProcessRuntime");

let providerDomain: string;

// this delay tries to correct the initialization delay between the web worker and the test driver
const valueChangedInterval = 500;
const mixedSubscriptionDelay = 1500;

let radioProvider: RadioProvider;
let providerQos: any;

const store: Record<string, any> = {};
store.numberOfStations = -1;

// eslint-disable-next-line @typescript-eslint/explicit-function-return-type
function createAttributeImpl<T>(key: string, defaultValue?: T) {
    if (defaultValue !== undefined) {
        store[key] = defaultValue;
    }
    return {
        get: (): T => store[key],
        set: (value: T) => {
            store[key] = value;
        }
    };
}

// eslint-disable-next-line @typescript-eslint/explicit-function-return-type
function createReadAttributeImpl<T>(key: string, defaultValue?: T) {
    store[key] = defaultValue;
    return {
        get: (): T => store[key]
    };
}

async function initializeTest(provisioningSuffix: string, providedDomain: string): Promise<any> {
    // set joynr provisioning
    providerDomain = providedDomain;
    // @ts-ignore
    provisioning.persistency = "localStorage";
    // @ts-ignore
    provisioning.channelId = `End2EndCommTestParticipantId${provisioningSuffix}`;

    joynr.selectRuntime(InProcessRuntime);
    await joynr.load(provisioning as any);

    providerQos = new joynr.types.ProviderQos({
        customParameters: [],
        priority: Date.now(),
        scope: joynr.types.ProviderScope.GLOBAL,
        supportsOnChangeSubscriptions: true
    });

    const isCountryEnum = (parameter: any): boolean =>
        typeof parameter === "object" &&
        Object.getPrototypeOf(parameter) instanceof joynr.JoynrObject &&
        parameter._typeName === Country.GERMANY._typeName;
    const checkEnumInputs = (opArgs: any): void => {
        let enumElement;
        if (!isCountryEnum(opArgs.enumInput)) {
            throw new Error(
                `Argument enumInput with value ${opArgs.enumInput} is not correctly typed ${Country.GERMANY._typeName}`
            );
        }
        for (enumElement in opArgs.enumArrayInput) {
            if (Object.prototype.hasOwnProperty.call(opArgs.enumArrayInput, enumElement)) {
                if (!isCountryEnum(opArgs.enumArrayInput[enumElement])) {
                    throw new Error(
                        `Argument enumInput with value ${opArgs.enumArrayInput[enumElement]} is not correctly typed ${
                            Country.GERMANY._typeName
                        }`
                    );
                }
            }
        }
    };

    function triggerBroadcastsInternal(opArgs: any): any {
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
                        const hierarchicPartitions: any[] = [];
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

    // build the provider
    radioProvider = joynr.providerBuilder.build<typeof RadioProvider, RadioProvider.RadioProviderImplementation>(
        RadioProvider,
        {
            numberOfStations: createAttributeImpl<number>("numberOfStations"),
            attrProvidedImpl: createAttributeImpl<boolean>("attrProvidedImpl"),
            mixedSubscriptions: createAttributeImpl<string>("mixedSubscriptions", "interval"),
            StartWithCapitalLetter: createAttributeImpl<boolean>("StartWithCapitalLetter", true),
            attributeTestingProviderInterface: createReadAttributeImpl<DatatypeForTestLibjoynr>(
                "attributeTestingProviderInterface"
            ),
            failingSyncAttribute: {
                get: () => {
                    throw new joynr.exceptions.ProviderRuntimeException({
                        detailMessage: "failure in failingSyncAttribute getter"
                    });
                }
            },
            failingAsyncAttribute: {
                get: (): Promise<number> => {
                    return Promise.reject(
                        new joynr.exceptions.ProviderRuntimeException({
                            detailMessage: "failure in failingSyncAttribute getter"
                        })
                    );
                }
            },
            isOn: createAttributeImpl<boolean>("inOn", true),
            enumAttribute: createAttributeImpl<Country>("enumAttribute", Country.GERMANY),
            enumArrayAttribute: createAttributeImpl<Country[]>("enumArrayAttribute", [Country.GERMANY]),
            byteBufferAttribute: createAttributeImpl<number[]>("byteBufferAttribute"),
            typeDefForStruct: createAttributeImpl<RadioStation>("typeDefForStruct"),
            typeDefForPrimitive: createAttributeImpl<number>("typeDefForPrimitive"),
            stringMapAttribute: createAttributeImpl<StringMap>("stringMapAttribute"),
            complexStructMapAttribute: createAttributeImpl<ComplexStructMap>("complexStructMapAttribute"),
            addFavoriteStation: (opArgs: { radioStation: string | RadioStation }) => {
                // retrieve radioStation name for both overloaded version
                const name = (opArgs.radioStation as RadioStation).name || (opArgs.radioStation as string);

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
                    if (name.match(/error/)) {
                        if (name.match(/ApplicationException/)) {
                            return Promise.reject(ErrorList.EXAMPLE_ERROR_1);
                        } else {
                            return Promise.reject(
                                new joynr.exceptions.ProviderRuntimeException({
                                    detailMessage: "example message async"
                                })
                            );
                        }
                    } else {
                        return Promise.resolve({
                            returnValue: !!name.match(/true/)
                        });
                    }
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
            },
            operationWithEnumsAsInputAndOutput: (opArgs: any) => {
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
            },
            operationWithMultipleOutputParameters: (opArgs: any) => {
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
            },
            operationWithEnumsAsInputAndEnumArrayAsOutput: (opArgs: any) => {
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
            },
            methodWithSingleArrayParameters: (opArgs: any) => {
                // the dummy implementation transforms the incoming double values into strings.
                const stringArrayOut = [];
                if (opArgs.doubleArrayArg !== undefined) {
                    for (const element in opArgs.doubleArrayArg) {
                        if (Object.prototype.hasOwnProperty.call(opArgs.doubleArrayArg, element)) {
                            stringArrayOut.push(opArgs.doubleArrayArg[element].toString());
                        }
                    }
                }
                return {
                    stringArrayOut
                };
            },
            methodWithByteBuffer: (opArgs: { input: any }) => {
                /* the dummy implementation returns the incoming byteBuffer
                 */

                return {
                    result: opArgs.input
                };
            },
            methodWithTypeDef: (opArgs: { typeDefStructInput: any; typeDefPrimitiveInput: any }) => {
                /* the dummy implementation returns the incoming data
                 */

                return {
                    typeDefStructOutput: opArgs.typeDefStructInput,
                    typeDefPrimitiveOutput: opArgs.typeDefPrimitiveInput
                };
            },
            methodWithComplexMap: () => {
                return;
            },
            methodProvidedImpl: (opArgs: { arg: any }) => ({
                returnValue: opArgs.arg
            }),
            triggerBroadcasts: triggerBroadcastsInternal,
            triggerBroadcastsWithPartitions: triggerBroadcastsInternal,
            methodFireAndForgetWithoutParams: () => {
                const broadcast = radioProvider.fireAndForgetCallArrived;
                const outputParams = broadcast.createBroadcastOutputParameters();
                outputParams.setMethodName("methodFireAndForgetWithoutParams");
                broadcast.fire(outputParams);
            },
            methodFireAndForget: () => {
                const broadcast = radioProvider.fireAndForgetCallArrived;
                const outputParams = broadcast.createBroadcastOutputParameters();
                outputParams.setMethodName("methodFireAndForget");
                broadcast.fire(outputParams);
            }
        }
    );

    providerQos.priority = Date.now();
    // register provider at the given providerDomain
    await joynr.registration.registerProvider(providerDomain, radioProvider, providerQos);
}

function startTest(): Promise<any> {
    // change attribute value of numberOfStations periodically
    joynr.util.LongTimer.setInterval(() => {
        radioProvider.numberOfStations.valueChanged(++store.numberOfStations);
    }, valueChangedInterval);

    joynr.util.LongTimer.setTimeout(() => {
        store.mixedSubscriptions = "valueChanged1";
        radioProvider.mixedSubscriptions.valueChanged(store.mixedSubscriptions);
        joynr.util.LongTimer.setTimeout(() => {
            store.mixedSubscriptions = "valueChanged2";
            radioProvider.mixedSubscriptions.valueChanged(store.mixedSubscriptions);
        }, 10);
    }, mixedSubscriptionDelay);
    return Promise.resolve(joynr.participantIdStorage.getParticipantId(providerDomain, radioProvider));
}
function terminateTest(): Promise<void> {
    return joynr.registration.unregisterProvider(providerDomain, radioProvider);
}

ChildProcessUtils.registerHandlers(initializeTest, startTest, terminateTest);
