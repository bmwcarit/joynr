/*eslint global-require: "off"*/
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

import * as IltUtil from "./IltUtil";
import testbase from "test-base";
const log = testbase.logging.log;

import ExtendedEnumerationWithPartlyDefinedValues from "../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/ExtendedEnumerationWithPartlyDefinedValues";
import ExtendedTypeCollectionEnumerationInTypeCollection from "../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/ExtendedTypeCollectionEnumerationInTypeCollection";
import Enumeration from "../generated-javascript/joynr/interlanguagetest/Enumeration";
import MapStringString from "../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/MapStringString";
import ArrayTypeDefStruct from "../generated-javascript/joynr/interlanguagetest/typeDefCollection/ArrayTypeDefStruct";
import joynr from "joynr";
import TestInterfaceProxy from "../generated-javascript/joynr/interlanguagetest/TestInterfaceProxy";
import WebSocketLibjoynrRuntime from "joynr/joynr/start/WebSocketLibjoynrRuntime";
import UdsLibJoynrRuntime from "joynr/joynr/start/UdsLibJoynrRuntime";

if (process.env.domain === undefined) {
    log("please pass a domain as argument");
    process.exit(0);
}
// eslint-disable-next-line @typescript-eslint/no-non-null-assertion
const domain: string = process.env.domain!;
log(`domain: ${domain}`);

// eslint-disable-next-line @typescript-eslint/no-non-null-assertion
const runtime: string = process.env.runtime!;
log(`runtime: ${runtime}`);

describe("Consumer test", () => {
    const provisioning = testbase.provisioning_common;

    if (process.env.runtime !== undefined) {
        if (runtime === "websocket") {
            // provisioning data are defined in test-base
            joynr.selectRuntime(WebSocketLibjoynrRuntime);
        } else if (runtime === "uds") {
            if (!process.env.udspath || !process.env.udsclientid || !process.env.udsconnectsleeptimems) {
                log("please pass udspath, udsclientid, udsconnectsleeptimems as argument");
                process.exit(1);
            }
            provisioning.uds = {
                socketPath: process.env.udspath,
                clientId: process.env.udsclientid,
                connectSleepTimeMs: Number(process.env.udsconnectsleeptimems)
            };
            joynr.selectRuntime(UdsLibJoynrRuntime);
        }
    }

    provisioning.persistency = {
        clearPersistency: true
    };
    let testInterfaceProxy: TestInterfaceProxy;
    let onErrorSpy: jest.Mock;

    async function loadJoynr(compressed: any) {
        log("Environment not yet setup");
        await joynr.load(provisioning);
        log("joynr started");
        const messagingQos = new joynr.messaging.MessagingQos({
            ttl: 60000,
            compress: compressed
        });
        log(`messagingQos - compressed = ${compressed}`);
        testInterfaceProxy = await joynr.proxyBuilder.build<TestInterfaceProxy>(TestInterfaceProxy, {
            domain,
            messagingQos
        });

        log("testInterface proxy build");
    }

    describe("with compressed joynr", () => {
        beforeAll(async () => {
            return await loadJoynr(true);
        });

        it("callMethodWithoutParametersCompressed", async () => {
            log("callMethodWithoutParametersCompressed");

            await testInterfaceProxy.methodWithoutParameters();
        });

        afterAll(async () => {
            await joynr.shutdown();
        });
    });

    describe("without compressed joynr", () => {
        beforeAll(async () => {
            return await loadJoynr(false);
        });

        beforeEach(() => {
            onErrorSpy = jest.fn();
        });

        it("proxy is defined", () => {
            expect(testInterfaceProxy).toBeDefined();
        });

        it("callMethodWithoutParameters", async () => {
            log("callMethodWithoutParameters");
            return await testInterfaceProxy.methodWithoutParameters();
        });

        it("callMethodWithoutInputParameter", async () => {
            log("callMethodWithoutInputParameter");
            const retObj = await testInterfaceProxy.methodWithoutInputParameter();
            expect(retObj).toBeDefined();
            expect(retObj.booleanOut).toBeDefined();
            expect(retObj.booleanOut).toBeTruthy();
        });

        it("callMethodWithoutOutputParameter", async () => {
            log("callMethodWithoutOutputParameter");
            const args = {
                booleanArg: false
            };
            await testInterfaceProxy.methodWithoutOutputParameter(args);
            log("callMethodWithoutOutputParameter - OK");
        });

        it("callMethodWithSinglePrimitiveParameters", async () => {
            log("callMethodWithSinglePrimitiveParameters");
            const args = {
                uInt16Arg: 32767
            };
            const retObj = await testInterfaceProxy.methodWithSinglePrimitiveParameters(args);

            expect(retObj).toBeDefined();
            expect(retObj.stringOut).toBeDefined();
            const x = 32767;
            expect(retObj.stringOut).toEqual(x.toString());
            log("callMethodWithSinglePrimitiveParameters - OK");
        });

        it("callMethodWithMultiplePrimitiveParameters", async () => {
            const arg2 = 47.11;
            log("callMethodWithMultiplePrimitiveParameters");
            const args = {
                int32Arg: 2147483647,
                floatArg: arg2,
                booleanArg: false
            };
            const retObj = await testInterfaceProxy.methodWithMultiplePrimitiveParameters(args);

            expect(retObj).toBeDefined();
            expect(retObj.doubleOut).toBeDefined();
            expect(retObj.stringOut).toBeDefined();
            const x = 2147483647;
            expect(retObj.doubleOut).toBeCloseTo(arg2);
            expect(retObj.stringOut).toEqual(x.toString());
            log("callMethodWithMultiplePrimitiveParameters - OK");
        });

        it("callMethodWithSingleArrayParameters", async () => {
            log("callMethodWithSingleArrayParameters");
            const doubleArray = IltUtil.create("DoubleArray");
            const args = {
                doubleArrayArg: doubleArray
            };
            const retObj = await testInterfaceProxy.methodWithSingleArrayParameters(args);

            expect(retObj).toBeDefined();
            expect(retObj.stringArrayOut).toBeDefined();
            expect(IltUtil.check("StringArray", retObj.stringArrayOut)).toBeTruthy();
            log("callMethodWithSingleArrayParameters - OK");
        });

        it("callMethodWithMultipleArrayParameters", async () => {
            log("callMethodWithMultipleArrayParameters");
            const args = {
                stringArrayArg: IltUtil.create("StringArray"),
                int8ArrayArg: IltUtil.create("ByteArray"),
                enumArrayArg: IltUtil.create("ExtendedInterfaceEnumInTypeCollectionArray"),
                structWithStringArrayArrayArg: IltUtil.create("StructWithStringArrayArray")
            };
            const retObj = await testInterfaceProxy.methodWithMultipleArrayParameters(args);

            expect(retObj).toBeDefined();
            expect(retObj.uInt64ArrayOut).toBeDefined();
            expect(retObj.structWithStringArrayArrayOut).toBeDefined();
            expect(retObj.structWithStringArrayArrayOut).not.toBeNull();
            expect(IltUtil.check("UInt64Array", retObj.uInt64ArrayOut)).toBeTruthy();
            expect(IltUtil.check("StructWithStringArrayArray", retObj.structWithStringArrayArrayOut)).toBeTruthy();
            log("callMethodWithMultipleArrayParameters - OK");
        });

        async function callProxyMethodWithParameter(testMethod: any, testValue: any) {
            log(`callProxyMethodWithParameter called with testValue = ${JSON.stringify(testValue)}`);
            const retObj = await testMethod(testValue);

            expect(retObj).toBeDefined();
            expect(Object.values(retObj)[0]).toBeDefined();
            log(`returned value: ${JSON.stringify(Object.values(retObj)[0])}`);
            expect(Object.values(retObj)[0]).toEqual(Object.values(testValue)[0]);
            return retObj;
        }

        it("callMethodWithSingleByteBufferParameter", async () => {
            const byteBufferArg = [-128, 0, 127];

            log("callMethodWithSingleByteBufferParameter");
            const args = {
                byteBufferIn: byteBufferArg
            };
            await callProxyMethodWithParameter(testInterfaceProxy.methodWithSingleByteBufferParameter, args);
            log("callMethodWithSingleByteBufferParameter - OK");
        });

        it("callMethodWithMultipleByteBufferParameters", async () => {
            const byteBufferArg1 = [-5, 125];
            const byteBufferArg2 = [78, 0];

            log("callMethodWithMultipleByteBufferParameters");
            const args = {
                byteBufferIn1: byteBufferArg1,
                byteBufferIn2: byteBufferArg2
            };
            const retObj = await testInterfaceProxy.methodWithMultipleByteBufferParameters(args);

            expect(retObj).toBeDefined();
            expect(retObj.byteBufferOut).toBeDefined();
            expect(IltUtil.cmpByteBuffers(retObj.byteBufferOut, byteBufferArg1.concat(byteBufferArg2))).toBeTruthy();
            log("callMethodWithMultipleByteBufferParameters - OK");
        });

        it("callMethodWithInt64TypeDefParameter", async () => {
            log("callMethodWithInt64TypeDefParameter");
            const args = {
                int64TypeDefIn: 1
            };

            await callProxyMethodWithParameter(testInterfaceProxy.methodWithInt64TypeDefParameter, args);
            log("callMethodWithInt64TypeDefParameter - OK");
        });

        it("callMethodWithStringTypeDefParameter", async () => {
            log("callMethodWithStringTypeDefParameter");
            const args = {
                stringTypeDefIn: "TypeDefTestString"
            };

            await callProxyMethodWithParameter(testInterfaceProxy.methodWithStringTypeDefParameter, args);
            log("callMethodWithStringTypeDefParameter - OK");
        });

        it("callMethodWithStructTypeDefParameter", async () => {
            log("callMethodWithStructTypeDefParameter");
            const args = {
                structTypeDefIn: IltUtil.create("BaseStruct")
            };

            await callProxyMethodWithParameter(testInterfaceProxy.methodWithStructTypeDefParameter, args);
            log("callMethodWithStructTypeDefParameter - OK");
        });

        it("callMethodWithMapTypeDefParameter", async () => {
            log("callMethodWithMapTypeDefParameter");
            const value = new MapStringString();
            for (let i = 1; i <= 3; i++) {
                value.put(`keyString${i}`, `valueString${i}`);
            }
            const args = {
                mapTypeDefIn: value
            };

            await callProxyMethodWithParameter(testInterfaceProxy.methodWithMapTypeDefParameter, args);
            log("callMethodWithMapTypeDefParameter - OK");
        });

        it("callMethodWithEnumTypeDefParameter", async () => {
            log("callMethodWithEnumTypeDefParameter");
            const args = {
                enumTypeDefIn: Enumeration.ENUM_0_VALUE_1
            };

            await callProxyMethodWithParameter(testInterfaceProxy.methodWithEnumTypeDefParameter, args);
            log("callMethodWithEnumTypeDefParameter - OK");
        });

        it("callMethodWithByteBufferTypeDefParameter", async () => {
            log("callMethodWithByteBufferTypeDefParameter");
            const args = {
                byteBufferTypeDefIn: IltUtil.create("ByteArray")
            };

            await callProxyMethodWithParameter(testInterfaceProxy.methodWithByteBufferTypeDefParameter, args);
            log("callMethodWithByteBufferTypeDefParameter - OK");
        });

        it("callMethodWithArrayTypeDefParameter", async () => {
            log("callMethodWithArrayTypeDefParameter");
            const stringArray = {
                typeDefStringArray: IltUtil.create("StringArray")
            };
            const arrayTypeDefArg = new ArrayTypeDefStruct(stringArray);
            const args = {
                arrayTypeDefIn: arrayTypeDefArg
            };

            await callProxyMethodWithParameter(testInterfaceProxy.methodWithArrayTypeDefParameter, args);
            log("callMethodWithArrayTypeDefParameter - OK");
        });

        it("callMethodWithSingleEnumParameters", async () => {
            log("callMethodWithSingleEnumParameters");
            const args = {
                enumerationArg:
                    ExtendedEnumerationWithPartlyDefinedValues.ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES
            };
            const retObj = await testInterfaceProxy.methodWithSingleEnumParameters(args);

            expect(retObj).toBeDefined();
            expect(retObj.enumerationOut).toBeDefined();
            expect(retObj.enumerationOut).toEqual(
                ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION
            );
            log("callMethodWithSingleEnumParameters - OK");
        });

        it("callMethodWithMultipleEnumParameters", async () => {
            const args = {
                enumerationArg: Enumeration.ENUM_0_VALUE_3,
                extendedEnumerationArg:
                    ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION
            };
            const retObj = await testInterfaceProxy.methodWithMultipleEnumParameters(args);

            expect(retObj).toBeDefined();
            expect(retObj.enumerationOut).toBeDefined();
            expect(retObj.extendedEnumerationOut).toBeDefined();
            expect(retObj.enumerationOut).toEqual(Enumeration.ENUM_0_VALUE_1);
            expect(retObj.extendedEnumerationOut).toEqual(
                ExtendedEnumerationWithPartlyDefinedValues.ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES
            );
            log("callMethodWithMultipleEnumParameters - OK");
        });

        it("callMethodWithSingleStructParameters", async () => {
            log("callMethodWithSingleStructParameters");
            const args = {
                extendedBaseStructArg: IltUtil.create("ExtendedBaseStruct")
            };
            const retObj = await testInterfaceProxy.methodWithSingleStructParameters(args);

            expect(retObj).toBeDefined();
            expect(retObj.extendedStructOfPrimitivesOut).toBeDefined();
            expect(IltUtil.checkExtendedStructOfPrimitives(retObj.extendedStructOfPrimitivesOut)).toBeTruthy();
            log("callMethodWithSingleStructParameters - OK");
        });

        it("callMethodWithMultipleStructParameters", async () => {
            log("callMethodWithMultipleStructParameters");
            const args = {
                extendedStructOfPrimitivesArg: IltUtil.createExtendedStructOfPrimitives(),
                // TODO
                // currently not supported:
                // anonymousBaseStructArg:
                baseStructArg: IltUtil.create("BaseStruct")
            };
            const retObj = await testInterfaceProxy.methodWithMultipleStructParameters(args);

            expect(retObj).toBeDefined();
            expect(retObj.baseStructWithoutElementsOut).toBeDefined();
            expect(retObj.extendedExtendedBaseStructOut).toBeDefined();
            expect(IltUtil.check("BaseStructWithoutElements", retObj.baseStructWithoutElementsOut)).toBeTruthy();
            expect(IltUtil.check("ExtendedExtendedBaseStruct", retObj.extendedExtendedBaseStructOut)).toBeTruthy();
            log("callMethodWithMultipleStructParameters - OK");
        });

        it("callMethodFireAndForgetWithoutParameter", done => {
            /*
             * FireAndForget methods do not have a return value and the calling proxy does not receive an answer to a fireAndForget method call.
             * The attribute attributeFireAndForget is used in fireAndForget method calls to check if the method is called at the provider.
             * The provider will change the attribute to a (fireAndForget) method specific value which will be checked in the subscription listener.
             */
            log("callMethodFireAndForgetWithoutParameter");
            let expected = -1;
            const subscriptionQosOnChange = new joynr.proxy.OnChangeSubscriptionQos({
                minIntervalMs: 50,
                validityMs: 60000
            });
            let attributeFireAndForgetSubscriptionId: any;

            // set attributeFireAndForget to 0 (it might have been set to the expected value by another test)
            log("callMethodFireAndForgetWithoutParameter - setAttributeFireAndForget");
            const args = {
                value: 0
            };
            testInterfaceProxy.attributeFireAndForget
                .set(args)
                .then(() => {
                    log("callMethodFireAndForgetWithoutParameter - setAttributeFireAndForget - OK");

                    // subscribe to attributeFireAndForget
                    log("callMethodFireAndForgetWithoutParameter - subscribeToAttributeFireAndForget");

                    return testInterfaceProxy.attributeFireAndForget.subscribe({
                        subscriptionQos: subscriptionQosOnChange,
                        onReceive,
                        onError: done.fail
                    });
                })
                .then((subscriptionId: string) => {
                    attributeFireAndForgetSubscriptionId = subscriptionId;
                    log(
                        `callMethodFireAndForgetWithoutParameter - subscribeToAttributeFireAndForget subscriptionId = ${attributeFireAndForgetSubscriptionId}`
                    );
                })
                .catch((error: any) => {
                    done.fail(
                        `callMethodFireAndForgetWithoutParameter - subscribeToAttributeFireAndForget - FAILED: ${error}`
                    );
                });

            let firstReceive = true;

            function onReceive(attributeFireAndForgetValue: any) {
                if (firstReceive) {
                    firstReceive = false;

                    expect(attributeFireAndForgetValue).toBeDefined();
                    expect(attributeFireAndForgetValue).toEqual(0);
                    log("callMethodFireAndForgetWithoutParameter - subscribeToAttributeFireAndForget - OK");

                    // call methodFireAndForgetWithoutParameter
                    expected = attributeFireAndForgetValue + 1;

                    log("callMethodFireAndForgetWithoutParameter CALL");
                    testInterfaceProxy.methodFireAndForgetWithoutParameter().catch((error: any) => {
                        done.fail(`callMethodFireAndForgetWithoutParameter CALL - FAILED: ${error}`);
                    });
                } else {
                    expect(attributeFireAndForgetValue).toBeDefined();
                    expect(attributeFireAndForgetValue).toEqual(expected);
                    log("callMethodFireAndForgetWithoutParameter - OK");

                    // unsubscribe again
                    log("callMethodFireAndForgetWithoutParameter - subscribeToAttributeFireAndForget unsubscribe");
                    testInterfaceProxy.attributeFireAndForget
                        .unsubscribe({
                            subscriptionId: attributeFireAndForgetSubscriptionId
                        })
                        .then(() => {
                            log(
                                "callMethodFireAndForgetWithoutParameter - subscribeToAttributeFireAndForget unsubscribe - OK"
                            );
                            log("callMethodFireAndForgetWithoutParameter - DONE");
                            // eslint-disable-next-line promise/no-callback-in-promise
                            done();
                        })
                        .catch((error: any) => {
                            done.fail(
                                `callMethodFireAndForgetWithoutParameter - subscribeToAttributeFireAndForget unsubscribe - FAILED: ${error}`
                            );
                        });
                }
            }
        });

        it("callMethodFireAndForgetWithInputParameter", done => {
            log("callMethodFireAndForgetWithInputParameter");
            let expected = -1;
            const subscriptionQosOnChange = new joynr.proxy.OnChangeSubscriptionQos({
                minIntervalMs: 50,
                validityMs: 60000
            });
            let attributeFireAndForgetSubscriptionId: any;

            // set attributeFireAndForget to 0 (it might have been set to the expected value by another test)
            log("callMethodFireAndForgetWithInputParameter - setAttributeFireAndForget");
            const args = {
                value: 0
            };
            testInterfaceProxy.attributeFireAndForget
                .set(args)
                .then(() => {
                    log("callMethodFireAndForgetWithInputParameter - setAttributeFireAndForget - OK");

                    // subscribe to attributeFireAndForget
                    log("callMethodFireAndForgetWithInputParameter - subscribeToAttributeFireAndForget");
                    /*eslint-disable no-use-before-define*/
                    return testInterfaceProxy.attributeFireAndForget.subscribe({
                        subscriptionQos: subscriptionQosOnChange,
                        onReceive,
                        onError: done.fail
                    });
                    /*eslint-enable no-use-before-define*/
                })
                .then((subscriptionId: any) => {
                    attributeFireAndForgetSubscriptionId = subscriptionId;
                    log(
                        `callMethodFireAndForgetWithInputParameter - subscribeToAttributeFireAndForget subscriptionId = ${attributeFireAndForgetSubscriptionId}`
                    );
                })
                .catch((error: any) => {
                    done.fail(
                        `callMethodFireAndForgetWithInputParameter - subscribeToAttributeFireAndForget - FAILED: ${error}`
                    );
                });

            let firstReceive = true;
            function onReceive(attributeFireAndForgetValue: any) {
                if (firstReceive) {
                    firstReceive = false;

                    expect(attributeFireAndForgetValue).toBeDefined();
                    expect(attributeFireAndForgetValue).toEqual(0);
                    log("callMethodFireAndForgetWithInputParameter - subscribeToAttributeFireAndForget - OK");

                    // call methodFireAndForgetWithInputParameter
                    expected = attributeFireAndForgetValue + 1;

                    log("callMethodFireAndForgetWithInputParameter CALL");
                    testInterfaceProxy.methodFireAndForgetWithoutParameter().catch((error: any) => {
                        done.fail(`callMethodFireAndForgetWithInputParameter CALL - FAILED: ${error}`);
                    });
                } else {
                    expect(attributeFireAndForgetValue).toBeDefined();
                    expect(attributeFireAndForgetValue).toEqual(expected);
                    log("callMethodFireAndForgetWithInputParameter - OK");

                    // unsubscribe again
                    log("callMethodFireAndForgetWithInputParameter - subscribeToAttributeFireAndForget unsubscribe");
                    testInterfaceProxy.attributeFireAndForget
                        .unsubscribe({
                            subscriptionId: attributeFireAndForgetSubscriptionId
                        })
                        .then(() => {
                            log(
                                "callMethodFireAndForgetWithInputParameter - subscribeToAttributeFireAndForget unsubscribe - OK"
                            );
                            log("callMethodFireAndForgetWithInputParameter - DONE");
                            // eslint-disable-next-line promise/no-callback-in-promise
                            done();
                        })
                        .catch((error: any) => {
                            done.fail(
                                `callMethodFireAndForgetWithInputParameter - subscribeToAttributeFireAndForget unsubscribe - FAILED: ${error}`
                            );
                        });
                }
            }
        });

        it("callOverloadedMethod_1", async () => {
            log("callOverloadedMethod_1");
            const retObj = await testInterfaceProxy.overloadedMethod();

            expect(retObj).toBeDefined();
            expect(retObj.stringOut).toBeDefined();
            expect(retObj.stringOut).toEqual("TestString 1");
            log("callOverloadedMethod_1 - OK");
        });

        it("callOverloadedMethod_2", async () => {
            const args = {
                booleanArg: false
            };
            log("callOverloadedMethod_2");
            const retObj = await testInterfaceProxy.overloadedMethod(args);

            expect(retObj).toBeDefined();
            expect(retObj.stringOut).toBeDefined();
            expect(retObj.stringOut).toEqual("TestString 2");
            log("callOverloadedMethod_2 - OK");
        });

        it("callOverloadedMethod_3", async () => {
            const args = {
                enumArrayArg: IltUtil.create("ExtendedExtendedEnumerationArray"),
                int64Arg: 1,
                baseStructArg: IltUtil.create("BaseStruct"),
                booleanArg: false
            };
            log("callOverloadedMethod_3");
            const retObj: TestInterfaceProxy.OverloadedMethodReturns3 = (await testInterfaceProxy.overloadedMethod(
                args
            )) as any;

            expect(retObj).toBeDefined();
            expect(retObj.doubleOut).toBeDefined();
            expect(retObj.stringArrayOut).toBeDefined();
            expect(retObj.extendedBaseStructOut).toBeDefined();
            expect(IltUtil.cmpDouble(retObj.doubleOut, 0.0)).toBeTruthy();
            expect(IltUtil.check("StringArray", retObj.stringArrayOut)).toBeTruthy();
            expect(IltUtil.check("ExtendedBaseStruct", retObj.extendedBaseStructOut)).toBeTruthy();
            log("callOverloadedMethod_3 - OK");
        });

        it("callOverloadedMethodWithSelector_1", async () => {
            log("callOverloadedMethodWithSelector_1");
            const retObj = await testInterfaceProxy.overloadedMethodWithSelector();

            expect(retObj).toBeDefined();
            expect(retObj.stringOut).toBeDefined();
            expect(retObj.stringOut).toEqual("Return value from overloadedMethodWithSelector 1");
            log("callOverloadedMethodWithSelector_1 - OK");
        });

        it("callOverloadedMethodWithSelector_2", async () => {
            const args = {
                booleanArg: false
            };
            log("callOverloadedMethodWithSelector_2");
            const retObj = await testInterfaceProxy.overloadedMethodWithSelector(args);

            expect(retObj).toBeDefined();
            expect(retObj.stringOut).toBeDefined();
            expect(retObj.stringOut).toEqual("Return value from overloadedMethodWithSelector 2");
            log("callOverloadedMethodWithSelector_2 - OK");
        });

        it("callOverloadedMethodWithSelector_3", async () => {
            const args = {
                enumArrayArg: IltUtil.create("ExtendedExtendedEnumerationArray"),
                int64Arg: 1,
                baseStructArg: IltUtil.create("BaseStruct"),
                booleanArg: false
            };
            log("callOverloadedMethodWithSelector_3");
            const retObj: TestInterfaceProxy.OverloadedMethodReturns3 = (await testInterfaceProxy.overloadedMethod(
                args
            )) as any;

            expect(retObj).toBeDefined();
            expect(retObj.doubleOut).toBeDefined();
            expect(retObj.stringArrayOut).toBeDefined();
            expect(retObj.extendedBaseStructOut).toBeDefined();
            expect(IltUtil.cmpDouble(retObj.doubleOut, 0.0)).toBeTruthy();
            expect(IltUtil.check("StringArray", retObj.stringArrayOut)).toBeTruthy();
            expect(IltUtil.check("ExtendedBaseStruct", retObj.extendedBaseStructOut)).toBeTruthy();
            log("callOverloadedMethodWithSelector_3 - OK");
        });

        it("callMethodWithStringsAndSpecifiedStringOutputLength", async () => {
            log("callMethodWithStringsAndSpecifiedStringOutputLength");
            const args = {
                stringArg: "Hello world",
                int32StringLengthArg: 32
            };
            const retObj = await testInterfaceProxy.methodWithStringsAndSpecifiedStringOutLength(args);

            expect(retObj).toBeDefined();
            expect(retObj.stringOut).toBeDefined();
            expect(retObj.stringOut.length).toEqual(32);
            log("callMethodWithStringsAndSpecifiedStringOutputLength - OK");
        });

        it("callMethodWithoutErrorEnum", async () => {
            log("callMethodWithoutErrorEnum");
            const args = {
                wantedExceptionArg: "ProviderRuntimeException"
            };
            expect.assertions(4);
            try {
                await testInterfaceProxy.methodWithoutErrorEnum(args);
            } catch (retObj) {
                expect(retObj).toBeDefined();
                expect(retObj._typeName).toEqual("joynr.exceptions.ProviderRuntimeException");
                expect(retObj.detailMessage).toBeDefined();
                expect(retObj.detailMessage).toEqual("Exception from methodWithoutErrorEnum");
                log("callMethodWithoutErrorEnum - OK");
            }
        });

        it("callMethodWithAnonymousErrorEnum", async () => {
            log("callMethodWithAnonymousErrorEnunm");
            let args = {
                wantedExceptionArg: "ProviderRuntimeException"
            };
            expect.assertions(8);
            try {
                await testInterfaceProxy.methodWithAnonymousErrorEnum(args);
            } catch (retObj) {
                expect(retObj._typeName).toEqual("joynr.exceptions.ProviderRuntimeException");
                expect(retObj.detailMessage).toBeDefined();
                expect(retObj.detailMessage).toEqual("Exception from methodWithAnonymousErrorEnum");
                log("callMethodWithAnonymousErrorEnunm - 1st - OK");
            }

            args = {
                wantedExceptionArg: "ApplicationException"
            };

            try {
                await testInterfaceProxy.methodWithAnonymousErrorEnum(args);
            } catch (retObj) {
                expect(retObj).toBeDefined();
                expect(retObj._typeName).toEqual("joynr.exceptions.ApplicationException");
                expect(retObj.error).toBeDefined();
                //expect(retObj.error).toEqual(MethodWithAnonymousErrorEnumErrorEnum.ERROR_3_1_NTC);
                expect(retObj.error._typeName).toEqual(
                    "joynr.interlanguagetest.TestInterface.MethodWithAnonymousErrorEnumErrorEnum"
                );
                expect(retObj.error.name).toEqual("ERROR_3_1_NTC");
                log("callMethodWithAnonymousErrorEnun - 2nd - OK");
            }
        });

        it("callMethodWithExistingErrorEnum", async () => {
            log("callMethodWithExistingErrorEnunm");
            let args = {
                wantedExceptionArg: "ProviderRuntimeException"
            };
            expect.assertions(14);

            try {
                await testInterfaceProxy.methodWithExistingErrorEnum(args);
            } catch (retObj) {
                expect(retObj).toBeDefined();
                expect(retObj._typeName).toEqual("joynr.exceptions.ProviderRuntimeException");
                expect(retObj.detailMessage).toBeDefined();
                expect(retObj.detailMessage).toEqual("Exception from methodWithExistingErrorEnum");
                log("callMethodWithExistingErrorEnunm - 1st - OK");
            }

            args = {
                wantedExceptionArg: "ApplicationException_1"
            };

            try {
                await testInterfaceProxy.methodWithExistingErrorEnum(args);
            } catch (retObj) {
                expect(retObj).toBeDefined();
                expect(retObj._typeName).toEqual("joynr.exceptions.ApplicationException");
                expect(retObj.error).toBeDefined();
                // following statement does not work
                //expect(retObj.error).toEqual(ExtendedErrorEnumTc.ERROR_2_3_TC2);
                expect(retObj.error._typeName).toEqual(
                    "joynr.interlanguagetest.namedTypeCollection2.ExtendedErrorEnumTc"
                );
                expect(retObj.error.name).toEqual("ERROR_2_3_TC2");
                log("callMethodWithExistingErrorEnun - 2nd - OK");
            }

            args = {
                wantedExceptionArg: "ApplicationException_2"
            };

            try {
                await testInterfaceProxy.methodWithExistingErrorEnum(args);
            } catch (retObj) {
                expect(retObj).toBeDefined();
                expect(retObj._typeName).toEqual("joynr.exceptions.ApplicationException");
                expect(retObj.error).toBeDefined();
                //expect(retObj.error).toEqual(ExtendedErrorEnumTc.ERROR_1_2_TC_2);
                expect(retObj.error._typeName).toEqual(
                    "joynr.interlanguagetest.namedTypeCollection2.ExtendedErrorEnumTc"
                );
                expect(retObj.error.name).toEqual("ERROR_1_2_TC_2");
                log("callMethodWithExistingErrorEnum - 3rd - OK");
            }
        });

        it("callMethodWithExtendedErrorEnum", async () => {
            log("callMethodWithExtendedErrorEnum");
            expect.assertions(14);
            let args = {
                wantedExceptionArg: "ProviderRuntimeException"
            };
            try {
                await testInterfaceProxy.methodWithExtendedErrorEnum(args);
            } catch (retObj) {
                expect(retObj).toBeDefined();
                expect(retObj._typeName).toEqual("joynr.exceptions.ProviderRuntimeException");
                expect(retObj.detailMessage).toBeDefined();
                expect(retObj.detailMessage).toEqual("Exception from methodWithExtendedErrorEnum");
                log("callMethodWithExtendedErrorEnum - 1st - OK");
            }

            args = {
                wantedExceptionArg: "ApplicationException_1"
            };

            try {
                await testInterfaceProxy.methodWithExtendedErrorEnum(args);
            } catch (retObj) {
                expect(retObj).toBeDefined();
                expect(retObj._typeName).toEqual("joynr.exceptions.ApplicationException");
                expect(retObj.error).toBeDefined();
                //expect(retObj.error).toEqual(ExtendedErrorEnumTc.ERROR_3_3_NTC);
                expect(retObj.error._typeName).toEqual(
                    "joynr.interlanguagetest.TestInterface.MethodWithExtendedErrorEnumErrorEnum"
                );
                expect(retObj.error.name).toEqual("ERROR_3_3_NTC");
                log("callMethodWithExtendedErrorEnum - 2nd - OK");
            }

            args = {
                wantedExceptionArg: "ApplicationException_2"
            };

            try {
                await testInterfaceProxy.methodWithExtendedErrorEnum(args);
            } catch (retObj) {
                expect(retObj).toBeDefined();
                expect(retObj._typeName).toEqual("joynr.exceptions.ApplicationException");
                expect(retObj.error).toBeDefined();
                //expect(retObj.error).toEqual(ExtendedErrorEnumTc.ERROR_2_1_TC2);
                expect(retObj.error._typeName).toEqual(
                    "joynr.interlanguagetest.TestInterface.MethodWithExtendedErrorEnumErrorEnum"
                );
                expect(retObj.error.name).toEqual("ERROR_2_1_TC2");
                log("callMethodWithExtendedErrorEnum - 3rd - OK");
            }
        });

        it("callGetAttributeWithExceptionFromGetter", async () => {
            log("callGetAttributeWithExceptionFromGetter");
            expect.assertions(4);
            try {
                await testInterfaceProxy.attributeWithExceptionFromGetter.get();
            } catch (retObj) {
                expect(retObj).toBeDefined();
                expect(retObj._typeName).toEqual("joynr.exceptions.ProviderRuntimeException");
                expect(retObj.detailMessage).toBeDefined();
                expect(retObj.detailMessage).toEqual("Exception from getAttributeWithExceptionFromGetter");
                log("callGetAttributeWithExceptionFromGetter - OK");
            }
        });

        it("callSetAttributeWithExceptionFromSetter", async () => {
            log("callSetAttributeWithExceptionFromSetter");
            const args = {
                value: false
            };
            expect.assertions(4);
            try {
                await testInterfaceProxy.attributeWithExceptionFromSetter.set(args);
            } catch (retObj) {
                expect(retObj).toBeDefined();
                expect(retObj._typeName).toEqual("joynr.exceptions.ProviderRuntimeException");
                expect(retObj.detailMessage).toBeDefined();
                expect(retObj.detailMessage).toEqual("Exception from setAttributeWithExceptionFromSetter");
                log("callSetAttributeWithExceptionFromSetter - OK");
            }
        });

        it("callMethodWithSingleMapParameters", async () => {
            log("callMethodWithSingleMapParameters");
            const mapArg = new MapStringString();
            for (let i = 1; i <= 3; i++) {
                mapArg.put(`keyString${i}`, `valueString${i}`);
            }
            const args = {
                mapArg
            };
            const retObj = await testInterfaceProxy.methodWithSingleMapParameters(args);
            expect(retObj).toBeDefined();
            expect(retObj.mapOut).toBeDefined();
            for (let i = 1; i <= 3; i++) {
                expect(retObj.mapOut.get(`valueString${i}`)).toEqual(`keyString${i}`);
            }
            log("callMethodWithSingleMapParameters - OK");
        });

        async function genericSetGet(testObj: any, testValue: any) {
            log(`genericSetGet called with testValue = ${JSON.stringify(testValue)}`);
            await testObj.set({ value: testValue });

            const retObj = await testObj.get();
            expect(retObj).toBeDefined();
            expect(retObj).toEqual(testValue);
        }

        it("callSetandGetAttributeMapStringString", async () => {
            log("callSetandGetAttributeMapStringString");
            const mapArg = new MapStringString();
            for (let i = 1; i <= 3; i++) {
                mapArg.put(`keyString${i}`, `valueString${i}`);
            }
            return await genericSetGet(testInterfaceProxy.attributeMapStringString, mapArg);
        });

        it("callSetandGetAttributeUInt8", async () => {
            log("callSetandGetAttributeUInt8");
            return await genericSetGet(testInterfaceProxy.attributeUInt8, 127);
        });

        it("callSetandGetAttributeDouble", async () => {
            log("callSetandGetAttributeDouble");
            return await genericSetGet(testInterfaceProxy.attributeDouble, 1.1);
        });

        it("callGetAttributeBooleanReadonly", async () => {
            log("callGetAttributeBooleanReadonly");
            const retObj = await testInterfaceProxy.attributeBooleanReadonly.get();
            expect(retObj).toBeDefined();
            expect(retObj).toBeTruthy();
        });

        it("callSetandGetAttributeStringNoSubscriptions", async () => {
            log("callSetandGetAttributeStringNoSubscriptions");
            return await genericSetGet(testInterfaceProxy.attributeStringNoSubscriptions, "Hello world");
        });

        it("callGetAttributeInt8readonlyNoSubscriptions", async () => {
            log("callGetAttributeInt8readonlyNoSubscriptions");
            const retObj = await testInterfaceProxy.attributeInt8readonlyNoSubscriptions.get();
            expect(retObj).toBeDefined();
            expect(retObj).toEqual(-128);
        });

        it("callSetandGetAttributeArrayOfStringImplicit", async () => {
            log("callSetandGetAttributeArrayOfStringImplicit");
            return await genericSetGet(
                testInterfaceProxy.attributeArrayOfStringImplicit,
                IltUtil.create("StringArray")
            );
        });

        it("callSetandGetAttributeByteBuffer", async () => {
            log("callSetandGetAttributeByteBuffer");
            return await genericSetGet(testInterfaceProxy.attributeByteBuffer, IltUtil.create("ByteArray"));
        });

        it("callSetandGetAttributeInt64TypeDef", async () => {
            log("callSetandGetAttributeInt64TypeDef");
            const testValue = 1;
            return await genericSetGet(testInterfaceProxy.attributeInt64TypeDef, testValue);
        });

        it("callSetandGetAttributeStringTypeDef", async () => {
            log("callSetandGetAttributeStringTypeDef");
            return await genericSetGet(testInterfaceProxy.attributeStringTypeDef, "StringTypeDef");
        });

        it("callSetandGetAttributeStructTypeDef", async () => {
            log("callSetandGetAttributeStructTypeDef");
            return await genericSetGet(testInterfaceProxy.attributeStructTypeDef, IltUtil.create("BaseStruct"));
        });

        it("callSetandGetAttributeMapTypeDef", async () => {
            log("callSetandGetAttributeMapTypeDef");
            const value = new MapStringString();
            for (let i = 1; i <= 3; i++) {
                value.put(`keyString${i}`, `valueString${i}`);
            }
            return await genericSetGet(testInterfaceProxy.attributeMapTypeDef, value);
        });

        it("callSetandGetAttributeEnumTypeDef", async () => {
            log("callSetandGetAttributeEnumTypeDef");
            return await genericSetGet(testInterfaceProxy.attributeEnumTypeDef, Enumeration.ENUM_0_VALUE_1);
        });

        it("callSetandGetAttributeByteBufferTypeDef", async () => {
            log("callSetandGetAttributeByteBufferTypeDef");
            return await genericSetGet(testInterfaceProxy.attributeByteBufferTypeDef, IltUtil.create("ByteArray"));
        });

        it("callSetandGetAttributeArrayTypeDef", async () => {
            log("callSetandGetAttributeArrayTypeDef");
            const args = {
                typeDefStringArray: IltUtil.create("StringArray")
            };
            const arrayTypeDefArg = new ArrayTypeDefStruct(args);

            return await genericSetGet(testInterfaceProxy.attributeArrayTypeDef, arrayTypeDefArg);
        });

        it("callSetandGetAttributeEnumeration", async () => {
            log("callSetandGetAttributeEnumeration");
            return await genericSetGet(testInterfaceProxy.attributeEnumeration, Enumeration.ENUM_0_VALUE_2);
        });

        it("callGetAttributeExtendedEnumerationReadonly", async () => {
            log("callGetAttributeExtendedEnumerationReadonly");
            const retObj = await testInterfaceProxy.attributeExtendedEnumerationReadonly.get();

            expect(retObj).toBeDefined();
            expect(retObj).toEqual(
                ExtendedEnumerationWithPartlyDefinedValues.ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES
            );
        });

        it("callSetandGetAttributeBaseStruct", async () => {
            log("callSetandGetAttributeBaseStruct");
            return await genericSetGet(testInterfaceProxy.attributeBaseStruct, IltUtil.create("BaseStruct"));
        });

        it("callSetandGetAttributeExtendedExtendedBaseStruct", async () => {
            log("callSetandGetAttributeExtendedExtendedBaseStruct");
            return await genericSetGet(
                testInterfaceProxy.attributeExtendedExtendedBaseStruct,
                IltUtil.create("ExtendedExtendedBaseStruct")
            );
        });

        it("callSubscribeAttributeEnumeration", async () => {
            const subscriptionQosOnChange = new joynr.proxy.OnChangeSubscriptionQos({
                minIntervalMs: 50,
                validityMs: 60000
            });

            const onSubscribedDeferred = IltUtil.createDeferred();
            const onReceiveDeferred = IltUtil.createDeferred();

            log("callSubscribeAttributeEnumeration");
            const subscriptionId = await testInterfaceProxy.attributeEnumeration.subscribe({
                subscriptionQos: subscriptionQosOnChange,
                onReceive: onReceiveDeferred.resolve,
                onError: onErrorSpy,
                onSubscribed: onSubscribedDeferred.resolve
            });
            const id = await onSubscribedDeferred.promise;
            expect(id).toEqual(subscriptionId);
            const retObj = await onReceiveDeferred.promise;
            expect(retObj).toBeDefined();
            expect(retObj).toEqual(Enumeration.ENUM_0_VALUE_2);
            await testInterfaceProxy.attributeEnumeration.unsubscribe({
                subscriptionId
            });
            expect(onErrorSpy).not.toHaveBeenCalled();
        });

        it("callSubscribeAttributeWithExceptionFromGetter", async () => {
            const subscriptionQosOnChange = new joynr.proxy.OnChangeSubscriptionQos({
                minIntervalMs: 50,
                validityMs: 60000
            });

            const onSubscribedDeferred = IltUtil.createDeferred();
            const onReceiveDeferred = IltUtil.createDeferred();

            log("callSubscribeAttributeWithExceptionFromGetter");
            const subscriptionId = await testInterfaceProxy.attributeWithExceptionFromGetter.subscribe({
                subscriptionQos: subscriptionQosOnChange,
                onReceive: onErrorSpy,
                onError: onReceiveDeferred.resolve,
                onSubscribed: onSubscribedDeferred.resolve
            });

            log(`callSubscribeAttributeWithExceptionFromGetter - subscriptionId = ${subscriptionId}`);
            const id = await onSubscribedDeferred.promise;
            expect(id).toEqual(subscriptionId);

            const retObj = await onReceiveDeferred.promise;
            log(`retObj = ${JSON.stringify(retObj)}`);
            expect(retObj).toBeDefined();
            expect(retObj._typeName).toEqual("joynr.exceptions.ProviderRuntimeException");
            expect(retObj.detailMessage).toBeDefined();
            expect(retObj.detailMessage).toEqual("Exception from getAttributeWithExceptionFromGetter");

            await testInterfaceProxy.attributeWithExceptionFromGetter.unsubscribe({
                subscriptionId
            });
            expect(onErrorSpy).not.toHaveBeenCalled();
        });

        async function callSubscribeBroadcastWithSinglePrimitiveParameter(partitionsToUse: any) {
            const multicastSubscriptionQos = new joynr.proxy.MulticastSubscriptionQos({
                validityMs: 60000
            });

            const onSubscribedDeferred = IltUtil.createDeferred();
            const onReceiveDeferred = IltUtil.createDeferred();

            log("callSubscribeBroadcastWithSinglePrimitiveParameter");
            const subscriptionId = await testInterfaceProxy.broadcastWithSinglePrimitiveParameter.subscribe({
                subscriptionQos: multicastSubscriptionQos,
                partitions: partitionsToUse,
                onReceive: (retObj: any) => {
                    expect(retObj).toBeDefined();
                    expect(retObj.stringOut).toBeDefined();
                    expect(retObj.stringOut).toEqual("boom");
                    log(`publication retObj: ${JSON.stringify(retObj)}`);

                    testInterfaceProxy.broadcastWithSinglePrimitiveParameter
                        .unsubscribe({
                            subscriptionId
                        })
                        .then(onReceiveDeferred.resolve)
                        .catch(onReceiveDeferred.reject);
                },
                onError: onErrorSpy,
                onSubscribed: onSubscribedDeferred.resolve
            });

            log(`subscriptionId = ${subscriptionId}`);
            const id = await onSubscribedDeferred.promise;
            expect(id).toEqual(subscriptionId);
            await testInterfaceProxy.methodToFireBroadcastWithSinglePrimitiveParameter({
                partitions: partitionsToUse
            });
            // call to fire broadcast went ok
            // now wait for the publication to happen
            await onReceiveDeferred.promise;
            expect(onErrorSpy).not.toHaveBeenCalled();
        }

        it("callSubscribeBroadcastWithSinglePrimitiveParameter_NoPartitions", async () => {
            return await callSubscribeBroadcastWithSinglePrimitiveParameter([]);
        });

        it("callSubscribeBroadcastWithSinglePrimitiveParameter_SimplePartitions", async () => {
            return await callSubscribeBroadcastWithSinglePrimitiveParameter(["partition0", "partition1"]);
        });

        async function callSubscribeBroadcastWithMultiplePrimitiveParameters(partitionsToUse: any) {
            const multicastSubscriptionQos = new joynr.proxy.MulticastSubscriptionQos({
                validityMs: 60000
            });

            const onSubscribedDeferred = IltUtil.createDeferred();
            const onReceiveDeferred = IltUtil.createDeferred();

            log("subscribeBroadcastWithMultiplePrimitiveParameters");
            const subscriptionId = await testInterfaceProxy.broadcastWithMultiplePrimitiveParameters.subscribe({
                subscriptionQos: multicastSubscriptionQos,
                partitions: partitionsToUse,
                onReceive: (retObj: any) => {
                    expect(retObj).toBeDefined();
                    expect(retObj.doubleOut).toBeDefined();
                    expect(IltUtil.cmpDouble(retObj.doubleOut, 1.1)).toBeTruthy();
                    expect(retObj.stringOut).toBeDefined();
                    expect(retObj.stringOut).toEqual("boom");
                    log(`publication retObj: ${JSON.stringify(retObj)}`);
                    onReceiveDeferred.resolve();
                },
                onError: onErrorSpy,
                onSubscribed: onSubscribedDeferred.resolve
            });

            log(`subscriptionId = ${subscriptionId}`);
            const id = await onSubscribedDeferred.promise;
            expect(id).toEqual(subscriptionId);
            await testInterfaceProxy.methodToFireBroadcastWithMultiplePrimitiveParameters({
                partitions: partitionsToUse
            });

            // call to fire broadcast went ok
            // now wait for the publication to happen
            await onReceiveDeferred.promise;
            await testInterfaceProxy.broadcastWithMultiplePrimitiveParameters.unsubscribe({
                subscriptionId
            });
            expect(onErrorSpy).not.toHaveBeenCalled();
        }

        it("callSubscribeBroadcastWithMultiplePrimitiveParameters_NoPartitions", async () => {
            return await callSubscribeBroadcastWithMultiplePrimitiveParameters([]);
        });

        it("callSubscribeBroadcastWithMultiplePrimitiveParameters_SimplePartitions", async () => {
            return await callSubscribeBroadcastWithMultiplePrimitiveParameters(["partition0", "partition1"]);
        });

        async function callSubscribeBroadcastWithSingleArrayParameter(partitionsToUse: any) {
            const multicastSubscriptionQos = new joynr.proxy.MulticastSubscriptionQos({
                validityMs: 60000
            });

            const onSubscribedDeferred = IltUtil.createDeferred();
            const onReceiveDeferred = IltUtil.createDeferred();

            log("callSubscribeBroadcastWithSingleArrayParameter");
            const subscriptionId = await testInterfaceProxy.broadcastWithSingleArrayParameter.subscribe({
                subscriptionQos: multicastSubscriptionQos,
                partitions: partitionsToUse,
                onReceive: (retObj: any) => {
                    expect(retObj).toBeDefined();
                    expect(retObj.stringArrayOut).toBeDefined();
                    expect(IltUtil.check("StringArray", retObj.stringArrayOut)).toBeTruthy();
                    log(`publication retObj: ${JSON.stringify(retObj)}`);
                    onReceiveDeferred.resolve();
                },
                onError: onErrorSpy,
                onSubscribed: onSubscribedDeferred.resolve
            });

            log(`subscriptionId = ${subscriptionId}`);
            const id = await onSubscribedDeferred.promise;
            expect(id).toEqual(subscriptionId);

            await testInterfaceProxy.methodToFireBroadcastWithSingleArrayParameter({
                partitions: partitionsToUse
            });

            // call to fire broadcast went ok
            // now wait for the publication to happen

            await onReceiveDeferred.promise;

            // unsubscribe again
            await testInterfaceProxy.broadcastWithSingleArrayParameter.unsubscribe({
                subscriptionId
            });
            expect(onErrorSpy).not.toHaveBeenCalled();
        }

        it("callSubscribeBroadcastWithSingleArrayParameter_NoPartitions", async () => {
            return await callSubscribeBroadcastWithSingleArrayParameter([]);
        });

        it("callSubscribeBroadcastWithSingleArrayParameter_SimplePartitions", async () => {
            return await callSubscribeBroadcastWithSingleArrayParameter(["partition0", "partition1"]);
        });

        async function callSubscribeBroadcastWithMultipleArrayParameters(partitionsToUse: any) {
            const multicastSubscriptionQos = new joynr.proxy.MulticastSubscriptionQos({
                validityMs: 60000
            });

            const onSubscribedDeferred = IltUtil.createDeferred();
            const onReceiveDeferred = IltUtil.createDeferred();

            log("callSubscribeBroadcastWithMultipleArrayParameters");
            const subscriptionId = await testInterfaceProxy.broadcastWithMultipleArrayParameters.subscribe({
                subscriptionQos: multicastSubscriptionQos,
                partitions: partitionsToUse,
                onReceive: (retObj: any) => {
                    expect(retObj).toBeDefined();
                    expect(retObj.uInt64ArrayOut).toBeDefined();
                    expect(IltUtil.check("UInt64Array", retObj.uInt64ArrayOut)).toBeTruthy();
                    expect(retObj.structWithStringArrayArrayOut).toBeDefined();
                    expect(
                        IltUtil.check("StructWithStringArrayArray", retObj.structWithStringArrayArrayOut)
                    ).toBeTruthy();
                    log(`publication retObj: ${JSON.stringify(retObj)}`);
                    onReceiveDeferred.resolve();
                },
                onError: onErrorSpy,
                onSubscribed: onSubscribedDeferred.resolve
            });

            log(`subscriptionId = ${subscriptionId}`);

            const id = await onSubscribedDeferred.promise;
            expect(id).toEqual(subscriptionId);

            await testInterfaceProxy.methodToFireBroadcastWithMultipleArrayParameters({
                partitions: partitionsToUse
            });
            // call to fire broadcast went ok
            // now wait for the publication to happen
            await onReceiveDeferred.promise;

            // unsubscribe again
            await testInterfaceProxy.broadcastWithMultipleArrayParameters.unsubscribe({
                subscriptionId
            });
            expect(onErrorSpy).not.toHaveBeenCalled();
        }

        it("callSubscribeBroadcastWithMultipleArrayParameters_NoPartitions", async () => {
            return await callSubscribeBroadcastWithMultipleArrayParameters([]);
        });

        it("callSubscribeBroadcastWithMultipleArrayParameters_SimplePartitions", async () => {
            return await callSubscribeBroadcastWithMultipleArrayParameters(["partition0", "partition1"]);
        });

        const byteBufferArg = [-128, 0, 127];

        async function callSubscribeBroadcastWithSingleByteBufferParameter(byteBufferArg: any, partitionsToUse: any) {
            const multicastSubscriptionQos = new joynr.proxy.MulticastSubscriptionQos({
                validityMs: 60000
            });

            const onSubscribedDeferred = IltUtil.createDeferred();
            const onReceiveDeferred = IltUtil.createDeferred();

            log("callSubscribeBroadcastWithSingleByteBufferParameter");
            const subscriptionId = await testInterfaceProxy.broadcastWithSingleByteBufferParameter.subscribe({
                subscriptionQos: multicastSubscriptionQos,
                partitions: partitionsToUse,
                onReceive: (retObj: any) => {
                    expect(retObj).toBeDefined();
                    expect(retObj.byteBufferOut).toBeDefined();
                    expect(IltUtil.cmpByteBuffers(retObj.byteBufferOut, byteBufferArg)).toBeTruthy();
                    log(`Successful publication of retObj: ${JSON.stringify(retObj)}`);
                    onReceiveDeferred.resolve();
                },
                onError: onErrorSpy,
                onSubscribed: onSubscribedDeferred.resolve
            });
            const id = await onSubscribedDeferred.promise;
            expect(id).toEqual(subscriptionId);

            log(`Subscription was successful with subscriptionId = ${subscriptionId}`);

            // execute fire method here
            testInterfaceProxy.methodToFireBroadcastWithSingleByteBufferParameter({
                byteBufferIn: byteBufferArg,
                partitions: partitionsToUse
            });

            await onReceiveDeferred.promise;

            // unsubscribe again
            await testInterfaceProxy.broadcastWithSingleByteBufferParameter.unsubscribe({
                subscriptionId
            });
            expect(onErrorSpy).not.toHaveBeenCalled();
        }

        it("callSubscribeBroadcastWithSingleByteBufferParameter_NoPartitions", async () => {
            return await callSubscribeBroadcastWithSingleByteBufferParameter(byteBufferArg, []);
        });

        it("callSubscribeBroadcastWithSingleByteBufferParameter_SimplePartitions", async () => {
            return await callSubscribeBroadcastWithSingleByteBufferParameter(byteBufferArg, [
                "partition0",
                "partition1"
            ]);
        });

        const byteBufferArg1 = [-5, 125];
        const byteBufferArg2 = [78, 0];

        async function callSubscribeBroadcastWithMultipleByteBufferParameters(
            byteBufferArg1: any,
            byteBufferArg2: any,
            partitionsToUse: any
        ) {
            const multicastSubscriptionQos = new joynr.proxy.MulticastSubscriptionQos({
                validityMs: 60000
            });

            const onSubscribedDeferred = IltUtil.createDeferred();
            const onReceiveDeferred = IltUtil.createDeferred();

            log("callSubscribeBroadcastWithMultipleByteBufferParameters");
            const subscriptionId = await testInterfaceProxy.broadcastWithMultipleByteBufferParameters.subscribe({
                subscriptionQos: multicastSubscriptionQos,
                partitions: partitionsToUse,
                onReceive: (retObj: any) => {
                    expect(retObj).toBeDefined();
                    expect(retObj.byteBufferOut1).toBeDefined();
                    expect(retObj.byteBufferOut2).toBeDefined();
                    expect(IltUtil.cmpByteBuffers(retObj.byteBufferOut1, byteBufferArg1)).toBeTruthy();
                    expect(IltUtil.cmpByteBuffers(retObj.byteBufferOut2, byteBufferArg2)).toBeTruthy();
                    log(`Successful publication of retObj: ${JSON.stringify(retObj)}`);
                    onReceiveDeferred.resolve();
                },
                onError: onErrorSpy,
                onSubscribed: onSubscribedDeferred.resolve
            });

            const id = await onSubscribedDeferred.promise;
            expect(id).toEqual(subscriptionId);

            log(`Subscription was successful with subscriptionId = ${subscriptionId}`);

            // execute fire method here
            await testInterfaceProxy.methodToFireBroadcastWithMultipleByteBufferParameters({
                byteBufferIn1: byteBufferArg1,
                byteBufferIn2: byteBufferArg2,
                partitions: partitionsToUse
            });

            await onReceiveDeferred.promise;

            // unsubscribe again
            await testInterfaceProxy.broadcastWithMultipleByteBufferParameters.unsubscribe({
                subscriptionId
            });
            expect(onErrorSpy).not.toHaveBeenCalled();

            log("Successfully unsubscribed from broadcast");
        }

        it("callSubscribeBroadcastWithMultipleByteBufferParameters_NoPartitions", async () => {
            return await callSubscribeBroadcastWithMultipleByteBufferParameters(byteBufferArg1, byteBufferArg2, []);
        });

        it("callSubscribeBroadcastWithMultipleByteBufferParameters_SimplePartitions", async () => {
            return await callSubscribeBroadcastWithMultipleByteBufferParameters(byteBufferArg1, byteBufferArg2, [
                "partition0",
                "partition1"
            ]);
        });

        async function callSubscribeBroadcastWithSingleEnumerationParameter(partitionsToUse: any) {
            const multicastSubscriptionQos = new joynr.proxy.MulticastSubscriptionQos({
                validityMs: 60000
            });

            const onSubscribedDeferred = IltUtil.createDeferred();
            const onReceiveDeferred = IltUtil.createDeferred();

            log("callSubscribeBroadcastWithSingleEnumerationParameter");
            const subscriptionId = await testInterfaceProxy.broadcastWithSingleEnumerationParameter.subscribe({
                subscriptionQos: multicastSubscriptionQos,
                partitions: partitionsToUse,
                onReceive: (retObj: any) => {
                    expect(retObj).toBeDefined();
                    expect(retObj.enumerationOut).toBeDefined();
                    expect(retObj.enumerationOut).toEqual(
                        ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION
                    );
                    log(`publication retObj: ${JSON.stringify(retObj)}`);
                    onReceiveDeferred.resolve();
                },
                onError: onErrorSpy,
                onSubscribed: onSubscribedDeferred.resolve
            });

            const id = await onSubscribedDeferred.promise;
            expect(id).toEqual(subscriptionId);
            log(`subscriptionId = ${subscriptionId}`);

            await testInterfaceProxy.methodToFireBroadcastWithSingleEnumerationParameter({
                partitions: partitionsToUse
            });

            // call to fire broadcast went ok
            // now wait for the publication to happen
            await onReceiveDeferred.promise;

            // unsubscribe again
            await testInterfaceProxy.broadcastWithSingleEnumerationParameter.unsubscribe({
                subscriptionId
            });
            expect(onErrorSpy).not.toHaveBeenCalled();
        }

        it("callSubscribeBroadcastWithSingleEnumerationParameter_NoPartitions", async () => {
            return await callSubscribeBroadcastWithSingleEnumerationParameter([]);
        });

        it("callSubscribeBroadcastWithSingleEnumerationParameter_SimplePartitions", async () => {
            return await callSubscribeBroadcastWithSingleEnumerationParameter(["partition0", "partition1"]);
        });

        async function callSubscribeBroadcastWithMultipleEnumerationParameter(partitionsToUse: any) {
            const multicastSubscriptionQos = new joynr.proxy.MulticastSubscriptionQos({
                validityMs: 60000
            });

            const onSubscribedDeferred = IltUtil.createDeferred();
            const onReceiveDeferred = IltUtil.createDeferred();

            log("callSubscribeBroadcastWithMultipleEnumerationParameters");
            const subscriptionId = await testInterfaceProxy.broadcastWithMultipleEnumerationParameters.subscribe({
                subscriptionQos: multicastSubscriptionQos,
                partitions: partitionsToUse,
                onReceive: (retObj: any) => {
                    expect(retObj).toBeDefined();
                    expect(retObj.extendedEnumerationOut).toBeDefined();
                    expect(retObj.extendedEnumerationOut).toEqual(
                        ExtendedEnumerationWithPartlyDefinedValues.ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES
                    );
                    expect(retObj.enumerationOut).toBeDefined();
                    expect(retObj.enumerationOut).toEqual(Enumeration.ENUM_0_VALUE_1);
                    onReceiveDeferred.resolve();
                },
                onError: onErrorSpy,
                onSubscribed: onSubscribedDeferred.resolve
            });

            const id = await onSubscribedDeferred.promise;
            expect(id).toEqual(subscriptionId);
            log(`subscriptionId = ${subscriptionId}`);

            testInterfaceProxy.methodToFireBroadcastWithMultipleEnumerationParameters({
                partitions: partitionsToUse
            });
            // call to fire broadcast went ok
            // now wait for the publication to happen
            await onReceiveDeferred.promise;

            // unsubscribe again
            await testInterfaceProxy.broadcastWithMultipleEnumerationParameters.unsubscribe({
                subscriptionId
            });
            expect(onErrorSpy).not.toHaveBeenCalled();
        }

        it("callSubscribeBroadcastWithMultipleEnumerationParameter_NoPartitions", async () => {
            return await callSubscribeBroadcastWithMultipleEnumerationParameter([]);
        });

        it("callSubscribeBroadcastWithMultipleEnumerationParameter_SimplePartitions", async () => {
            return await callSubscribeBroadcastWithMultipleEnumerationParameter(["partition0", "partition1"]);
        });

        async function callSubscribeBroadcastWithSingleStructParameter(partitionsToUse: any) {
            const multicastSubscriptionQos = new joynr.proxy.MulticastSubscriptionQos({
                validityMs: 60000
            });

            const onSubscribedDeferred = IltUtil.createDeferred();
            const onReceiveDeferred = IltUtil.createDeferred();

            log("callSubscribeBroadcastWithSingleStructParameter");
            const subscriptionId = await testInterfaceProxy.broadcastWithSingleStructParameter.subscribe({
                subscriptionQos: multicastSubscriptionQos,
                partitions: partitionsToUse,
                onReceive: (retObj: any) => {
                    expect(retObj).toBeDefined();
                    expect(retObj.extendedStructOfPrimitivesOut).toBeDefined();
                    expect(IltUtil.checkExtendedStructOfPrimitives(retObj.extendedStructOfPrimitivesOut)).toBeTruthy();
                    log(`publication retObj: ${JSON.stringify(retObj)}`);
                    onReceiveDeferred.resolve();
                },
                onError: onErrorSpy,
                onSubscribed: onSubscribedDeferred.resolve
            });

            const id = await onSubscribedDeferred.promise;
            expect(id).toEqual(subscriptionId);
            log(`subscriptionId = ${subscriptionId}`);

            await testInterfaceProxy.methodToFireBroadcastWithSingleStructParameter({
                partitions: partitionsToUse
            });
            // call to fire broadcast went ok
            // now wait for the publication to happen
            await onReceiveDeferred.promise;

            // unsubscribe again
            await testInterfaceProxy.broadcastWithSingleStructParameter.unsubscribe({
                subscriptionId
            });
            expect(onErrorSpy).not.toHaveBeenCalled();
        }

        it("callSubscribeBroadcastWithSingleStructParameter_NoPartitions", async () => {
            return await callSubscribeBroadcastWithSingleStructParameter([]);
        });

        it("callSubscribeBroadcastWithSingleStructParameter_SimplePartitions", async () => {
            return await callSubscribeBroadcastWithSingleStructParameter(["partition0", "partition1"]);
        });

        async function callSubscribeBroadcastWithMultipleStructParameter(partitionsToUse: any) {
            const multicastSubscriptionQos = new joynr.proxy.MulticastSubscriptionQos({
                validityMs: 60000
            });

            const onSubscribedDeferred = IltUtil.createDeferred();
            const onReceiveDeferred = IltUtil.createDeferred();

            log("callSubscribeBroadcastWithMultipleStructParameters");
            const subscriptionId = await testInterfaceProxy.broadcastWithMultipleStructParameters.subscribe({
                subscriptionQos: multicastSubscriptionQos,
                partitions: partitionsToUse,
                onReceive: (retObj: any) => {
                    log(`XXX: publication retObj: ${JSON.stringify(retObj)}`);
                    expect(retObj).toBeDefined();
                    expect(retObj.baseStructWithoutElementsOut).toBeDefined();
                    expect(
                        IltUtil.check("BaseStructWithoutElements", retObj.baseStructWithoutElementsOut)
                    ).toBeTruthy();
                    expect(retObj.extendedExtendedBaseStructOut).toBeDefined();
                    expect(
                        IltUtil.check("ExtendedExtendedBaseStruct", retObj.extendedExtendedBaseStructOut)
                    ).toBeTruthy();
                    onReceiveDeferred.resolve();
                },
                onError: onErrorSpy,
                onSubscribed: onSubscribedDeferred.resolve
            });

            const id = await onSubscribedDeferred.promise;
            expect(id).toEqual(subscriptionId);
            log(`subscriptionId = ${subscriptionId}`);

            await testInterfaceProxy.methodToFireBroadcastWithMultipleStructParameters({
                partitions: partitionsToUse
            });

            // call to fire broadcast went ok
            // now wait for the publication to happen
            await onReceiveDeferred.promise;

            // unsubscribe again
            await testInterfaceProxy.broadcastWithMultipleStructParameters.unsubscribe({
                subscriptionId
            });
            expect(onErrorSpy).not.toHaveBeenCalled();
        }

        it("callSubscribeBroadcastWithMultipleStructParameter_NoPartitions", async () => {
            return await callSubscribeBroadcastWithMultipleStructParameter([]);
        });

        it("callSubscribeBroadcastWithMultipleStructParameter_SimplePartitions", async () => {
            return await callSubscribeBroadcastWithMultipleStructParameter(["partition0", "partition1"]);
        });

        // different pattern compared to the rest of the tests
        it("doNotReceivePublicationsForOtherPartitions", async () => {
            const subscriptionQosOnChange = new joynr.proxy.OnChangeSubscriptionQos({
                minIntervalMs: 50,
                validityMs: 60000
            });

            const onSubscribedDeferred = IltUtil.createDeferred();
            const onReceiveDeferred = IltUtil.createDeferred();
            const onReceivedSpy = jest.fn();

            const subscribeToPartitions = ["partition0", "partition1"];
            const broadcastPartition = ["otherPartition"];

            log("doNotReceivePublicationsForOtherPartitions");
            const subscriptionId = await testInterfaceProxy.broadcastWithSingleEnumerationParameter.subscribe({
                subscriptionQos: subscriptionQosOnChange,
                partitions: subscribeToPartitions,
                onReceive: (retObj: any) => {
                    onReceivedSpy(retObj);
                    onReceiveDeferred.resolve();
                },
                onError: onErrorSpy,
                onSubscribed: onSubscribedDeferred.resolve
            });

            const id = await onSubscribedDeferred.promise;
            expect(id).toEqual(subscriptionId);
            log(`subscriptionId = ${subscriptionId}`);

            await testInterfaceProxy.methodToFireBroadcastWithSingleEnumerationParameter({
                partitions: broadcastPartition
            });

            // The partitions do not match. Expect no broadcast
            expect(onReceivedSpy).not.toHaveBeenCalled();

            await testInterfaceProxy.methodToFireBroadcastWithSingleEnumerationParameter({
                partitions: subscribeToPartitions
            });

            await onReceiveDeferred.promise;
            await testInterfaceProxy.broadcastWithMultipleStructParameters.unsubscribe({
                subscriptionId
            });
            expect(onErrorSpy).not.toHaveBeenCalled();
        });

        it("callSubscribeBroadcastWithFiltering", async () => {
            const subscriptionQosOnChange = new joynr.proxy.OnChangeSubscriptionQos({
                minIntervalMs: 50,
                validityMs: 60000
            });

            const onSubscribedDeferred = IltUtil.createDeferred();
            const onReceiveDeferred = IltUtil.createDeferred();

            log("callSubscribeBroadcastWithFiltering");
            const filterParameters = testInterfaceProxy.broadcastWithFiltering.createFilterParameters();
            const stringOfInterest = "fireBroadcast";
            filterParameters.setStringOfInterest(stringOfInterest);
            //filterParameters.setStringArrayOfInterest(JSON.stringify(IltUtil.createStringArray()));
            //filterParameters.setEnumerationOfInterest(JSON.stringify(ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION));
            //filterParameters.setStructWithStringArrayOfInterest(JSON.stringify(IltUtil.createStructWithStringArray()));
            //filterParameters.setStructWithStringArrayArrayOfInterest(JSON.stringify(IltUtil.createStructWithStringArrayArray()));
            filterParameters.setStringArrayOfInterest('["Hello","World"]');
            filterParameters.setEnumerationOfInterest('"ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION"');
            filterParameters.setStructWithStringArrayOfInterest(
                '{"_typeName":"joynr.interlanguagetest.namedTypeCollection1.StructWithStringArray","stringArrayElement":["Hello","World"]}'
            );
            filterParameters.setStructWithStringArrayArrayOfInterest(
                '[{"_typeName":"joynr.interlanguagetest.namedTypeCollection1.StructWithStringArray","stringArrayElement":["Hello","World"]},{"_typeName":"joynr.interlanguagetest.namedTypeCollection1.StructWithStringArray","stringArrayElement":["Hello","World"]}]'
            );

            const subscriptionId = await testInterfaceProxy.broadcastWithFiltering.subscribe({
                subscriptionQos: subscriptionQosOnChange,
                onReceive: (retObj: any) => {
                    onReceiveDeferred.resolve(retObj);
                },
                onError: onErrorSpy,
                onSubscribed: onSubscribedDeferred.resolve,
                filterParameters
            });

            const id = await onSubscribedDeferred.promise;
            expect(id).toEqual(subscriptionId);
            log(`subscriptionId = ${subscriptionId}`);

            // execute fire method here
            const args = {
                stringArg: "fireBroadcast"
            };
            await testInterfaceProxy.methodToFireBroadcastWithFiltering(args);

            // call to fire broadcast went ok
            // now wait for the publication to happen

            const retObj = await onReceiveDeferred.promise;

            log(`XXX: publication retObj: ${JSON.stringify(retObj)}`);
            expect(retObj).toBeDefined();
            expect(retObj.stringOut).toBeDefined();
            expect(retObj.stringOut).toEqual("fireBroadcast");
            expect(retObj.enumerationOut).toBeDefined();
            expect(retObj.enumerationOut).toEqual(
                ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION
            );
            expect(retObj.stringArrayOut).toBeDefined();
            expect(IltUtil.check("StringArray", retObj.stringArrayOut)).toBeTruthy();
            expect(retObj.structWithStringArrayOut).toBeDefined();
            expect(IltUtil.check("StructWithStringArray", retObj.structWithStringArrayOut)).toBeTruthy();
            expect(retObj.structWithStringArrayArrayOut).toBeDefined();
            expect(IltUtil.check("StructWithStringArrayArray", retObj.structWithStringArrayArrayOut)).toBeTruthy();

            // unsubscribe again
            await testInterfaceProxy.broadcastWithFiltering.unsubscribe({
                subscriptionId
            });
            expect(onErrorSpy).not.toHaveBeenCalled();
        });

        afterAll(async () => {
            await joynr.shutdown();
        });
    });
});
