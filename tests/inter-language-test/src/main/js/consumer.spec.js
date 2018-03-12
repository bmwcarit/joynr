/*jslint node: true */
/*jslint es5: true, nomen: true */

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

var IltUtil = require("./IltUtil.js");

var testbase = require("test-base");
var log = testbase.logging.log;
var prettyLog = testbase.logging.prettyLog;

var ExtendedEnumerationWithPartlyDefinedValues = require("../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/ExtendedEnumerationWithPartlyDefinedValues.js");
var ExtendedInterfaceEnumerationInTypeCollection = require("../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/ExtendedInterfaceEnumerationInTypeCollection.js");
var ExtendedTypeCollectionEnumerationInTypeCollection = require("../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/ExtendedTypeCollectionEnumerationInTypeCollection.js");
var Enumeration = require("../generated-javascript/joynr/interlanguagetest/Enumeration.js");
var MethodWithAnonymousErrorEnumErrorEnum = require("../generated-javascript/joynr/interlanguagetest/TestInterface/MethodWithAnonymousErrorEnumErrorEnum.js");
var ExtendedErrorEnumTc = require("../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/ExtendedErrorEnumTc.js")
var MethodWithExtendedErrorEnumErrorEnum = require("../generated-javascript/joynr/interlanguagetest/TestInterface/MethodWithExtendedErrorEnumErrorEnum.js");
var MapStringString = require("../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/MapStringString.js");

var Promise = require("bluebird").Promise;

jasmine.getEnv().addReporter(new testbase.TestReporter());

if (process.env.domain === undefined) {
    log("please pass a domain as argument");
    process.exit(0);
}
var domain = process.env.domain;
log("domain: " + domain);

describe("Consumer test", function() {

    var joynr = require("joynr");
    var provisioning = testbase.provisioning_common;
    var initialized = false;
    var testFinished = false;
    var compressed = true;
    var testInterfaceProxy;

    beforeEach(function() {
        var ready = false;

        if (initialized === false) {

            runs(function() {
                log("Environment not yet setup");
                joynr.load(provisioning).then(function(loadedJoynr) {
                    log("joynr started");
                    joynr = loadedJoynr;
                    var messagingQos = new joynr.messaging.MessagingQos({
                        ttl : 60000,
                        compress: compressed
                    });
                    log("messagingQos - compressed = " + compressed);
                    var TestInterfaceProxy = require("../generated-javascript/joynr/interlanguagetest/TestInterfaceProxy.js");
                    joynr.proxyBuilder.build(TestInterfaceProxy, {
                        domain : domain,
                        messagingQos : messagingQos
                    }).then(function(newTestInterfaceProxy) {
                        testInterfaceProxy = newTestInterfaceProxy;
                        log("testInterface proxy build");
                        ready = true;
                    }).catch(function(error) {
                        log("error building testInterfaceProxy: " + error);
                    });
                    return loadedJoynr;
                }).catch(function(error) {
                    throw error;
                });
            });

            waitsFor(function() {
                return ready;
            }, "joynr proxy built", 5000);

            runs(function() {
                initialized = true;
            });
        } else {
            log("Environment already setup");
        }
    });

    it("proxy is defined", function() {
        expect(testInterfaceProxy).toBeDefined();
    });

    it("callMethodWithoutParametersCompressed", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();

        runs(function() {
            log("callMethodWithoutParametersCompressed");
            testInterfaceProxy.methodWithoutParameters().then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callMethodWithoutParametersCompressed", 5000);

        runs(function() {
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            initialized = false;
            compressed = false;
        });
    });

    it("callMethodWithoutParameters", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();

        runs(function() {
            log("callMethodWithoutParameters");
            testInterfaceProxy.methodWithoutParameters().then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callMethodWithoutParameters", 5000);

        runs(function() {
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
        });
    });

    it("callMethodWithoutInputParameter", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();

        runs(function() {
            log("callMethodWithoutInputParameter");
            testInterfaceProxy.methodWithoutInputParameter().then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callMethodWithoutInputParameter", 5000);

        runs(function() {
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            var retObj = spy.onFulfilled.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj.booleanOut).toBeDefined();
            expect(retObj.booleanOut).toBeTruthy();
        });
    });

    it("callMethodWithoutOutputParameter", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();

        runs(function() {
            log("callMethodWithoutOutputParameter");
            var args = {
                booleanArg: false
            };
            testInterfaceProxy.methodWithoutOutputParameter(args).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callMethodWithoutOutputParameter", 5000);

        runs(function() {
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            log("callMethodWithoutOutputParameter - OK");
        });
    });

    it("callMethodWithSinglePrimitiveParameters", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();
        runs(function() {
            log("callMethodWithSinglePrimitiveParameters");
            var args = {
                uInt16Arg: 32767
            };
            testInterfaceProxy.methodWithSinglePrimitiveParameters(args).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callMethodWithSinglePrimitiveParameters", 5000);

        runs(function() {
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            var retObj = spy.onFulfilled.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj.stringOut).toBeDefined();
            var x = 32767;
            expect(retObj.stringOut).toEqual(x.toString());
            log("callMethodWithSinglePrimitiveParameters - OK");
        });
    });

    it("callMethodWithMultiplePrimitiveParameters", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();
        var arg2 = 47.11;
        runs(function() {
            log("callMethodWithMultiplePrimitiveParameters");
            var args = {
                int32Arg: 2147483647,
                floatArg: arg2,
                booleanArg: false
            };
            testInterfaceProxy.methodWithMultiplePrimitiveParameters(args).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callMethodWithMultiplePrimitiveParameters", 5000);

        runs(function() {
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            var retObj = spy.onFulfilled.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj.doubleOut).toBeDefined();
            expect(retObj.stringOut).toBeDefined();
            var x = 2147483647;
            expect(retObj.doubleOut).toBeCloseTo(arg2);
            expect(retObj.stringOut).toEqual(x.toString());
            log("callMethodWithMultiplePrimitiveParameters - OK");
        });
    });

    it("callMethodWithSingleArrayParameters", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();
        runs(function() {
            log("callMethodWithSingleArrayParameters");
            var doubleArray = IltUtil.createDoubleArray();
            var args = {
                doubleArrayArg: doubleArray
            };
            testInterfaceProxy.methodWithSingleArrayParameters(args).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callMethodWithSingleArrayParameters", 5000);

        runs(function() {
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            var retObj = spy.onFulfilled.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj.stringArrayOut).toBeDefined();
            expect(IltUtil.checkStringArray(retObj.stringArrayOut)).toBeTruthy();
            log("callMethodWithSingleArrayParameters - OK");
        });
    });

    it("callMethodWithMultipleArrayParameters", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();
        runs(function() {
            log("callMethodWithMultipleArrayParameters");
            var args = {
                stringArrayArg: IltUtil.createStringArray(),
                int8ArrayArg: IltUtil.createByteArray(),
                enumArrayArg: IltUtil.createExtendedInterfaceEnumerationInTypeCollectionArray(),
                structWithStringArrayArrayArg: IltUtil.createStructWithStringArrayArray()
            };
            testInterfaceProxy.methodWithMultipleArrayParameters(args).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callMethodWithMultipleArrayParameters", 5000);

        runs(function() {
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            var retObj = spy.onFulfilled.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj.uInt64ArrayOut).toBeDefined();
            expect(retObj.structWithStringArrayArrayOut).toBeDefined();
            expect(retObj.structWithStringArrayArrayOut).not.toBeNull();
            expect(IltUtil.checkUInt64Array(retObj.uInt64ArrayOut)).toBeTruthy();
            expect(IltUtil.checkStructWithStringArrayArray(retObj.structWithStringArrayArrayOut)).toBeTruthy();
            log("callMethodWithMultipleArrayParameters - OK");
        });
    });

    it("callMethodWithSingleEnumParameters", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();
        runs(function() {
            log("callMethodWithSingleEnumParameters");
            var args = {
                enumerationArg: ExtendedEnumerationWithPartlyDefinedValues.ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES
            };
            testInterfaceProxy.methodWithSingleEnumParameters(args).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callMethodWithSingleEnumParameters", 5000);

        runs(function() {
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            var retObj = spy.onFulfilled.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj.enumerationOut).toBeDefined();
            expect(retObj.enumerationOut).toEqual(ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION);
            log("callMethodWithSingleEnumParameters - OK");
        });
    });

    it("callMethodWithMultipleEnumParameters", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();
        runs(function() {
            log("callMethodWithMultipleEnumParameters");
            var args = {
                enumerationArg: Enumeration.ENUM_0_VALUE_3,
                extendedEnumerationArg: ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION
            };
            testInterfaceProxy.methodWithMultipleEnumParameters(args).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callMethodWithMultipleEnumParameters", 5000);

        runs(function() {
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            var retObj = spy.onFulfilled.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj.enumerationOut).toBeDefined();
            expect(retObj.extendedEnumerationOut).toBeDefined();
            expect(retObj.enumerationOut).toEqual(Enumeration.ENUM_0_VALUE_1);
            expect(retObj.extendedEnumerationOut).toEqual(ExtendedEnumerationWithPartlyDefinedValues.ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES);
            log("callMethodWithMultipleEnumParameters - OK");
        });
    });

    it("callMethodWithSingleStructParameters", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();
        runs(function() {
            log("callMethodWithSingleStructParameters");
            var args = {
                extendedBaseStructArg: IltUtil.createExtendedBaseStruct()
            };
            testInterfaceProxy.methodWithSingleStructParameters(args).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callMethodWithSingleStructParameters", 5000);

        runs(function() {
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            var retObj = spy.onFulfilled.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj.extendedStructOfPrimitivesOut).toBeDefined();
            expect(IltUtil.checkExtendedStructOfPrimitives(retObj.extendedStructOfPrimitivesOut)).toBeTruthy();
            log("callMethodWithSingleStructParameters - OK");
        });
    });

    it("callMethodWithMultipleStructParameters", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();
        runs(function() {
            log("callMethodWithMultipleStructParameters");
            var args = {
                extendedStructOfPrimitivesArg: IltUtil.createExtendedStructOfPrimitives(),
                // TODO
                // currently not supported:
                // anonymousBaseStructArg:
                baseStructArg: IltUtil.createBaseStruct()
            };
            testInterfaceProxy.methodWithMultipleStructParameters(args).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callMethodWithMultipleStructParameters", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            var retObj = spy.onFulfilled.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj.baseStructWithoutElementsOut).toBeDefined();
            expect(retObj.extendedExtendedBaseStructOut).toBeDefined();
            expect(IltUtil.checkBaseStructWithoutElements(retObj.baseStructWithoutElementsOut)).toBeTruthy();
            expect(IltUtil.checkExtendedExtendedBaseStruct(retObj.extendedExtendedBaseStructOut)).toBeTruthy();
            log("callMethodWithMultipleStructParameters - OK");
        });
    });

    it("callMethodFireAndForgetWithoutParameter", function() {
       /*
        * FireAndForget methods do not have a return value and the calling proxy does not receive an answer to a fireAndForget method call.
        * The attribute attributeFireAndForget is used in fireAndForget method calls to check if the method is called at the provider.
        * The provider will change the attribute to a (fireAndForget) method specific value which will be checked in the subscription listener.
        */
        log("callMethodFireAndForgetWithoutParameter");
        var spy = jasmine.createSpyObj("spy", [ "onPublication", "onPublicationError" ]);
        var expected = -1;
        var subscriptionQosOnChange = new joynr.proxy.OnChangeSubscriptionQos({ minIntervalMs: 50, validityMs: 60000 });
        var attributeFireAndForgetValue = -1;
        var attributeFireAndForgetSubscriptionId;

        runs(() => {
            // set attributeFireAndForget to 0 (it might have been set to the expected value by another test)
            log("callMethodFireAndForgetWithoutParameter - setAttributeFireAndForget");
            var args = {
                value: 0
            };
            testInterfaceProxy.attributeFireAndForget.set(args)
            .then(() => {
                log("callMethodFireAndForgetWithoutParameter - setAttributeFireAndForget - OK");

                // subscribe to attributeFireAndForget
                log("callMethodFireAndForgetWithoutParameter - subscribeToAttributeFireAndForget");
                return testInterfaceProxy.attributeFireAndForget.subscribe({
                        "subscriptionQos": subscriptionQosOnChange,
                        "onReceive": spy.onPublication,
                        "onError": spy.onPublicationError
                    });
            })
            .then((subscriptionId) => {
                attributeFireAndForgetSubscriptionId = subscriptionId;
                log("callMethodFireAndForgetWithoutParameter - subscribeToAttributeFireAndForget subscriptionId = " + attributeFireAndForgetSubscriptionId);
            })
            .catch((error) => {
                log("callMethodFireAndForgetWithoutParameter - subscribeToAttributeFireAndForget - FAILED: " + error);
                expect("callMethodFireAndForgetWithoutParameter - subscribeToAttributeFireAndForget - FAILED: " + error).toBeFalsy();
            });
        });

        waitsFor(() => {
            return attributeFireAndForgetSubscriptionId !== undefined;
        }, "callMethodFireAndForgetWithoutParameter - get attributeFireAndForgetSubscriptionId", 5000);

        waitsFor(() => {
            return spy.onPublication.callCount > 0;
        }, "callMethodFireAndForgetWithoutParameter - subscribeToAttributeFireAndForget initial Publication", 5000);

        runs(() => {
            attributeFireAndForgetValue = spy.onPublication.calls[0].args[0];
            expect(attributeFireAndForgetValue).toBeDefined();
            expect(attributeFireAndForgetValue).toEqual(0);
            log("callMethodFireAndForgetWithoutParameter - subscribeToAttributeFireAndForget - OK");

            // call methodFireAndForgetWithoutParameter
            expected = attributeFireAndForgetValue + 1;
            spy.onPublication.reset();
            spy.onPublicationError.reset();

            log("callMethodFireAndForgetWithoutParameter CALL");
            testInterfaceProxy.methodFireAndForgetWithoutParameter()
            .catch((error) => {
                log("callMethodFireAndForgetWithoutParameter CALL - FAILED: " + error);
                expect("callMethodFireAndForgetWithoutParameter CALL - FAILED: " + error).toBeFalsy();
            });
        });

        waitsFor(() => {
            return spy.onPublication.callCount > 0;
        }, "callMethodFireAndForgetWithoutParameter Publication", 5000);

        runs(() => {
            attributeFireAndForgetValue = spy.onPublication.calls[0].args[0];
            expect(attributeFireAndForgetValue).toBeDefined();
            expect(attributeFireAndForgetValue).toEqual(expected);
            log("callMethodFireAndForgetWithoutParameter - OK");

            // unsubscribe again
            log("callMethodFireAndForgetWithoutParameter - subscribeToAttributeFireAndForget unsubscribe");
            testInterfaceProxy.attributeFireAndForget.unsubscribe({
                "subscriptionId": attributeFireAndForgetSubscriptionId
            })
            .then(() => {
                log("callMethodFireAndForgetWithoutParameter - subscribeToAttributeFireAndForget unsubscribe - OK");
                log("callMethodFireAndForgetWithoutParameter - DONE");
            })
            .catch((error) => {
                log("callMethodFireAndForgetWithoutParameter - subscribeToAttributeFireAndForget unsubscribe - FAILED: " + error);
                expect("callMethodFireAndForgetWithoutParameter - subscribeToAttributeFireAndForget unsubscribe - FAILED: " + error).toBeFalsy();
            });
        });
    });

    it("callMethodFireAndForgetWithInputParameter", function() {
        log("callMethodFireAndForgetWithInputParameter");
        var spy = jasmine.createSpyObj("spy", [ "onPublication", "onPublicationError" ]);
        var expected = -1;
        var subscriptionQosOnChange = new joynr.proxy.OnChangeSubscriptionQos({ minIntervalMs: 50, validityMs: 60000 });
        var attributeFireAndForgetValue = -1;
        var attributeFireAndForgetSubscriptionId;

        runs(() => {
            // set attributeFireAndForget to 0 (it might have been set to the expected value by another test)
            log("callMethodFireAndForgetWithInputParameter - setAttributeFireAndForget");
            var args = {
                value: 0
            };
            testInterfaceProxy.attributeFireAndForget.set(args)
            .then(() => {
                log("callMethodFireAndForgetWithInputParameter - setAttributeFireAndForget - OK");

                // subscribe to attributeFireAndForget
                log("callMethodFireAndForgetWithInputParameter - subscribeToAttributeFireAndForget");
                return testInterfaceProxy.attributeFireAndForget.subscribe({
                        "subscriptionQos": subscriptionQosOnChange,
                        "onReceive": spy.onPublication,
                        "onError": spy.onPublicationError
                    });
            })
            .then((subscriptionId) => {
                attributeFireAndForgetSubscriptionId = subscriptionId;
                log("callMethodFireAndForgetWithInputParameter - subscribeToAttributeFireAndForget subscriptionId = " + attributeFireAndForgetSubscriptionId);
            })
            .catch((error) => {
                log("callMethodFireAndForgetWithInputParameter - subscribeToAttributeFireAndForget - FAILED: " + error);
                expect("callMethodFireAndForgetWithInputParameter - subscribeToAttributeFireAndForget - FAILED: " + error).toBeFalsy();
            });
        });

        waitsFor(() => {
            return attributeFireAndForgetSubscriptionId !== undefined;

        }, "callMethodFireAndForgetWithInputParameter - get attributeFireAndForgetSubscriptionId", 5000);

        waitsFor(() => {
            return spy.onPublication.callCount > 0
        }, "callMethodFireAndForgetWithInputParameter - subscribeToAttributeFireAndForget initial Publication", 5000);

        runs(() => {
            attributeFireAndForgetValue = spy.onPublication.calls[0].args[0];
            expect(attributeFireAndForgetValue).toBeDefined();
            expect(attributeFireAndForgetValue).toEqual(0);
            log("callMethodFireAndForgetWithInputParameter - subscribeToAttributeFireAndForget - OK");

            // call methodFireAndForgetWithInputParameter
            expected = attributeFireAndForgetValue + 1;
            spy.onPublication.reset();
            spy.onPublicationError.reset();

            log("callMethodFireAndForgetWithInputParameter CALL");
            testInterfaceProxy.methodFireAndForgetWithoutParameter()
            .catch(() => {
                log("callMethodFireAndForgetWithInputParameter CALL - FAILED: " + error);
                expect("callMethodFireAndForgetWithInputParameter CALL - FAILED: " + error).toBeFalsy();
            });
        });

        waitsFor(() => {
            return spy.onPublication.callCount > 0;
        }, "callMethodFireAndForgetWithInputParameter Publication", 5000);

        runs(() => {
            attributeFireAndForgetValue = spy.onPublication.calls[0].args[0];
            expect(attributeFireAndForgetValue).toBeDefined();
            expect(attributeFireAndForgetValue).toEqual(expected);
            log("callMethodFireAndForgetWithInputParameter - OK");

            // unsubscribe again
            log("callMethodFireAndForgetWithInputParameter - subscribeToAttributeFireAndForget unsubscribe");
            testInterfaceProxy.attributeFireAndForget.unsubscribe({
                "subscriptionId": attributeFireAndForgetSubscriptionId
            })
            .then(() => {
                log("callMethodFireAndForgetWithInputParameter - subscribeToAttributeFireAndForget unsubscribe - OK");
                log("callMethodFireAndForgetWithInputParameter - DONE");
            })
            .catch((error) => {
                log("callMethodFireAndForgetWithInputParameter - subscribeToAttributeFireAndForget unsubscribe - FAILED: " + error);
                expect("callMethodFireAndForgetWithInputParameter - subscribeToAttributeFireAndForget unsubscribe - FAILED: " + error).toBeFalsy();
            });
        });
    });

    it("callOverloadedMethod_1", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();
        runs(function() {
            log("callOverloadedMethod_1");
            testInterfaceProxy.overloadedMethod().then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callOverloadedMethod_1", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            var retObj = spy.onFulfilled.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj.stringOut).toBeDefined();
            expect(retObj.stringOut).toEqual("TestString 1");
            log("callOverloadedMethod_1 - OK");
        });
    });

    it("callOverloadedMethod_2", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();
        runs(function() {
            var args = {
                booleanArg: false
            };
            log("callOverloadedMethod_2");
            testInterfaceProxy.overloadedMethod(args).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callOverloadedMethod_2", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            var retObj = spy.onFulfilled.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj.stringOut).toBeDefined();
            expect(retObj.stringOut).toEqual("TestString 2");
            log("callOverloadedMethod_2 - OK");
        });
    });

    it("callOverloadedMethod_3", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();
        runs(function() {
            var args = {
                enumArrayArg: IltUtil.createExtendedExtendedEnumerationArray(),
                int64Arg: 1,
                baseStructArg: IltUtil.createBaseStruct(),
                booleanArg: false
            };
            log("callOverloadedMethod_3");
            testInterfaceProxy.overloadedMethod(args).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callOverloadedMethod_3", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            var retObj = spy.onFulfilled.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj.doubleOut).toBeDefined();
            expect(retObj.stringArrayOut).toBeDefined();
            expect(retObj.extendedBaseStructOut).toBeDefined();
            expect(IltUtil.cmpDouble(retObj.doubleOut, 0.0)).toBeTruthy();
            expect(IltUtil.checkStringArray(retObj.stringArrayOut)).toBeTruthy();
            expect(IltUtil.checkExtendedBaseStruct(retObj.extendedBaseStructOut)).toBeTruthy();
            log("callOverloadedMethod_3 - OK");
        });
    });

    it("callOverloadedMethodWithSelector_1", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();
        runs(function() {
            log("callOverloadedMethodWithSelector_1");
            testInterfaceProxy.overloadedMethodWithSelector().then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callOverloadedMethodWithSelector_1", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            var retObj = spy.onFulfilled.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj.stringOut).toBeDefined();
            expect(retObj.stringOut).toEqual("Return value from overloadedMethodWithSelector 1");
            log("callOverloadedMethodWithSelector_1 - OK");
        });
    });

    it("callOverloadedMethodWithSelector_2", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();
        runs(function() {
            var args = {
                booleanArg: false
            };
            log("callOverloadedMethodWithSelector_2");
            testInterfaceProxy.overloadedMethodWithSelector(args).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callOverloadedMethodWithSelector_2", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            var retObj = spy.onFulfilled.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj.stringOut).toBeDefined();
            expect(retObj.stringOut).toEqual("Return value from overloadedMethodWithSelector 2");
            log("callOverloadedMethodWithSelector_2 - OK");
        });
    });

    it("callOverloadedMethodWithSelector_3", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();
        runs(function() {
            var args = {
                enumArrayArg: IltUtil.createExtendedExtendedEnumerationArray(),
                int64Arg: 1,
                baseStructArg: IltUtil.createBaseStruct(),
                booleanArg: false
            };
            log("callOverloadedMethodWithSelector_3");
            testInterfaceProxy.overloadedMethod(args).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callOverloadedMethodWithSelector_3", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            var retObj = spy.onFulfilled.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj.doubleOut).toBeDefined();
            expect(retObj.stringArrayOut).toBeDefined();
            expect(retObj.extendedBaseStructOut).toBeDefined();
            expect(IltUtil.cmpDouble(retObj.doubleOut, 0.0)).toBeTruthy();
            expect(IltUtil.checkStringArray(retObj.stringArrayOut)).toBeTruthy();
            expect(IltUtil.checkExtendedBaseStruct(retObj.extendedBaseStructOut)).toBeTruthy();
            log("callOverloadedMethodWithSelector_3 - OK");
        });
    });

    it("callMethodWithStringsAndSpecifiedStringOutputLength", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();
        runs(function() {
            log("callMethodWithStringsAndSpecifiedStringOutputLength");
            var args = {
                stringArg: "Hello world",
                int32StringLengthArg: 32
            };
            testInterfaceProxy.methodWithStringsAndSpecifiedStringOutLength(args).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callMethodWithStringsAndSpecifiedStringOutputLength", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            var retObj = spy.onFulfilled.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj.stringOut).toBeDefined();
            expect(retObj.stringOut.length).toEqual(32);
            log("callMethodWithStringsAndSpecifiedStringOutputLength - OK");
        });
    });

    it("callMethodWithoutErrorEnum", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();
        runs(function() {
            log("callMethodWithoutErrorEnum");
            var args = {
                wantedExceptionArg: "ProviderRuntimeException"
            };
            testInterfaceProxy.methodWithoutErrorEnum(args).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callMethodWithoutErrorEnum", 5000);

        runs(function() {
            //if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
            //    log(spy.onError.calls[0].args[0]);
            //}
            expect(spy.onFulfilled.callCount).toEqual(0);
            expect(spy.onError.callCount).toEqual(1);
            var retObj = spy.onError.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj._typeName).toEqual("joynr.exceptions.ProviderRuntimeException");
            expect(retObj.detailMessage).toBeDefined();
            expect(retObj.detailMessage).toEqual("Exception from methodWithoutErrorEnum");
            log("callMethodWithoutErrorEnum - OK");
        });
    });

    it("callMethodWithAnonymousErrorEnum", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();
        runs(function() {
            log("callMethodWithAnonymousErrorEnunm");
            var args = {
                wantedExceptionArg: "ProviderRuntimeException"
            };
            testInterfaceProxy.methodWithAnonymousErrorEnum(args).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callMethodWithAnonymousErrorEnum", 5000);

        runs(function() {
            expect(spy.onFulfilled.callCount).toEqual(0);
            expect(spy.onError.callCount).toEqual(1);
            var retObj = spy.onError.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj._typeName).toEqual("joynr.exceptions.ProviderRuntimeException");
            expect(retObj.detailMessage).toBeDefined();
            expect(retObj.detailMessage).toEqual("Exception from methodWithAnonymousErrorEnum");
            log("callMethodWithAnonymousErrorEnunm - 1st - OK");

            spy.onFulfilled.reset();
            spy.onError.reset();
            var args = {
                wantedExceptionArg: "ApplicationException"
            };
            testInterfaceProxy.methodWithAnonymousErrorEnum(args).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callMethodWithAnonymousErrorEnunm", 5000);

        runs(function() {
            expect(spy.onFulfilled.callCount).toEqual(0);
            expect(spy.onError.callCount).toEqual(1);
            var retObj = spy.onError.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj._typeName).toEqual("joynr.exceptions.ApplicationException");
            expect(retObj.error).toBeDefined();
            //expect(retObj.error).toEqual(MethodWithAnonymousErrorEnumErrorEnum.ERROR_3_1_NTC);
            expect(retObj.error._typeName).toEqual("joynr.interlanguagetest.TestInterface.MethodWithAnonymousErrorEnumErrorEnum");
            expect(retObj.error.name).toEqual("ERROR_3_1_NTC");
            log("callMethodWithAnonymousErrorEnun - 2nd - OK");
        });
    });

    it("callMethodWithExistingErrorEnum", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();
        runs(function() {
            log("callMethodWithExistingErrorEnunm");
            var args = {
                wantedExceptionArg: "ProviderRuntimeException"
            };
            testInterfaceProxy.methodWithExistingErrorEnum(args).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callMethodWithExistingErrorEnum", 5000);

        runs(function() {
            expect(spy.onFulfilled.callCount).toEqual(0);
            expect(spy.onError.callCount).toEqual(1);
            var retObj = spy.onError.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj._typeName).toEqual("joynr.exceptions.ProviderRuntimeException");
            expect(retObj.detailMessage).toBeDefined();
            expect(retObj.detailMessage).toEqual("Exception from methodWithExistingErrorEnum");
            log("callMethodWithExistingErrorEnunm - 1st - OK");

            spy.onFulfilled.reset();
            spy.onError.reset();
            var args = {
                wantedExceptionArg: "ApplicationException_1"
            };
            testInterfaceProxy.methodWithExistingErrorEnum(args).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callMethodWithExistingErrorEnunm", 5000);

        runs(function() {
            expect(spy.onFulfilled.callCount).toEqual(0);
            expect(spy.onError.callCount).toEqual(1);
            var retObj = spy.onError.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj._typeName).toEqual("joynr.exceptions.ApplicationException");
            expect(retObj.error).toBeDefined();
            // following statement does not work
            //expect(retObj.error).toEqual(ExtendedErrorEnumTc.ERROR_2_3_TC2);
            expect(retObj.error._typeName).toEqual("joynr.interlanguagetest.namedTypeCollection2.ExtendedErrorEnumTc");
            expect(retObj.error.name).toEqual("ERROR_2_3_TC2");
            log("callMethodWithExistingErrorEnun - 2nd - OK");

            spy.onFulfilled.reset();
            spy.onError.reset();
            var args = {
                wantedExceptionArg: "ApplicationException_2"
            };
            testInterfaceProxy.methodWithExistingErrorEnum(args).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callMethodWithExistingErrorEnunm", 5000);

        runs(function() {
            expect(spy.onFulfilled.callCount).toEqual(0);
            expect(spy.onError.callCount).toEqual(1);
            var retObj = spy.onError.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj._typeName).toEqual("joynr.exceptions.ApplicationException");
            expect(retObj.error).toBeDefined();
            //expect(retObj.error).toEqual(ExtendedErrorEnumTc.ERROR_1_2_TC_2);
            expect(retObj.error._typeName).toEqual("joynr.interlanguagetest.namedTypeCollection2.ExtendedErrorEnumTc");
            expect(retObj.error.name).toEqual("ERROR_1_2_TC_2");
            log("callMethodWithExistingErrorEnum - 3rd - OK");
        });
    });

    it("callMethodWithExtendedErrorEnum", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();
        runs(function() {
            log("callMethodWithExtendedErrorEnum");
            var args = {
                wantedExceptionArg: "ProviderRuntimeException"
            };
            testInterfaceProxy.methodWithExtendedErrorEnum(args).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callMethodWithExtendedErrorEnum", 5000);

        runs(function() {
            expect(spy.onFulfilled.callCount).toEqual(0);
            expect(spy.onError.callCount).toEqual(1);
            var retObj = spy.onError.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj._typeName).toEqual("joynr.exceptions.ProviderRuntimeException");
            expect(retObj.detailMessage).toBeDefined();
            expect(retObj.detailMessage).toEqual("Exception from methodWithExtendedErrorEnum");
            log("callMethodWithExtendedErrorEnum - 1st - OK");

            spy.onFulfilled.reset();
            spy.onError.reset();
            var args = {
                wantedExceptionArg: "ApplicationException_1"
            };
            testInterfaceProxy.methodWithExtendedErrorEnum(args).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callMethodWithExtendedErrorEnum", 5000);

        runs(function() {
            expect(spy.onFulfilled.callCount).toEqual(0);
            expect(spy.onError.callCount).toEqual(1);
            var retObj = spy.onError.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj._typeName).toEqual("joynr.exceptions.ApplicationException");
            expect(retObj.error).toBeDefined();
            //expect(retObj.error).toEqual(ExtendedErrorEnumTc.ERROR_3_3_NTC);
            expect(retObj.error._typeName).toEqual("joynr.interlanguagetest.TestInterface.MethodWithExtendedErrorEnumErrorEnum");
            expect(retObj.error.name).toEqual("ERROR_3_3_NTC");
            log("callMethodWithExtendedErrorEnum - 2nd - OK");

            spy.onFulfilled.reset();
            spy.onError.reset();
            var args = {
                wantedExceptionArg: "ApplicationException_2"
            };
            testInterfaceProxy.methodWithExtendedErrorEnum(args).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callMethodWithExtendedErrorEnum", 5000);

        runs(function() {
            expect(spy.onFulfilled.callCount).toEqual(0);
            expect(spy.onError.callCount).toEqual(1);
            var retObj = spy.onError.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj._typeName).toEqual("joynr.exceptions.ApplicationException");
            expect(retObj.error).toBeDefined();
            //expect(retObj.error).toEqual(ExtendedErrorEnumTc.ERROR_2_1_TC2);
            expect(retObj.error._typeName).toEqual("joynr.interlanguagetest.TestInterface.MethodWithExtendedErrorEnumErrorEnum");
            expect(retObj.error.name).toEqual("ERROR_2_1_TC2");
            log("callMethodWithExtendedErrorEnum - 3rd - OK");
        });
    });

    it("callGetAttributeWithExceptionFromGetter", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();

        runs(function() {
            log("callGetAttributeWithExceptionFromGetter");
            var args = {
                value: false
            };
            testInterfaceProxy.attributeWithExceptionFromGetter.get(args).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callGetAttributeWithExceptionFromGetter", 5000);

        runs(function() {
            expect(spy.onFulfilled.callCount).toEqual(0);
            expect(spy.onError.callCount).toEqual(1);
            var retObj = spy.onError.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj._typeName).toEqual("joynr.exceptions.ProviderRuntimeException");
            expect(retObj.detailMessage).toBeDefined();
            expect(retObj.detailMessage).toEqual("Exception from getAttributeWithExceptionFromGetter");
            log("callGetAttributeWithExceptionFromGetter - OK");
        });
    });

    it("callSetAttributeWithExceptionFromSetter", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();

        runs(function() {
            log("callSetAttributeWithExceptionFromSetter");
            var args = {
                value: false
            };
            testInterfaceProxy.attributeWithExceptionFromSetter.set(args).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callSetAttributeWithExceptionFromSetter", 5000);

        runs(function() {
            expect(spy.onFulfilled.callCount).toEqual(0);
            expect(spy.onError.callCount).toEqual(1);
            var retObj = spy.onError.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj._typeName).toEqual("joynr.exceptions.ProviderRuntimeException");
            expect(retObj.detailMessage).toBeDefined();
            expect(retObj.detailMessage).toEqual("Exception from setAttributeWithExceptionFromSetter");
            log("callSetAttributeWithExceptionFromSetter - OK");
        });
    });

    it("callSetAttributeMapStringString", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();

        runs(function() {
            log("callSetAttributeMapStringString");
            var value = new MapStringString();
            for (var i = 1; i <= 3; i++) {
                value.put("keyString" + i, "valueString" + i);
            }
            var args = {
                value: value
            };
            testInterfaceProxy.attributeMapStringString.set(args).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callSetAttributeMapStringString", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
        });
    });

    it("callGetAttributeMapStringString", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();

        runs(function() {
            log("callGetAttributeMapStringString");
            testInterfaceProxy.attributeMapStringString.get().then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callGetAttributeMapStringString", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            var retObj = spy.onFulfilled.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj).toBeDefined();
            log("result = " + JSON.stringify(retObj));
            for (var i = 1; i <= 3; i++) {
                expect(retObj.get("keyString" + i)).toEqual("valueString" + i)
            }
        });
    });

    it("callMethodWithSingleMapParameters", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();
        runs(function() {
            log("callMethodWithSingleMapParameters");
            var mapArg = new MapStringString();
            for (var i = 1; i <= 3; i++) {
                mapArg.put("keyString" + i, "valueString" + i);
            }
            var args = {
                mapArg: mapArg
            };
            testInterfaceProxy.methodWithSingleMapParameters(args).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callMethodWithSingleMapParameters", 5000);

        runs(function() {
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            var retObj = spy.onFulfilled.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj.mapOut).toBeDefined();
            for (var i = 1; i <= 3; i++) {
                expect(retObj.mapOut.get("valueString" + i)).toEqual("keyString" + i)
            }
            log("callMethodWithSingleMapParameters - OK");
        });
    });

    it("callSetAttributeUInt8", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();

        runs(function() {
            log("callSetAttributeUInt8");
            var args = {
                value: 127
            };
            testInterfaceProxy.attributeUInt8.set(args).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callSetAttributeUInt8", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
        });
    });

    it("callGetAttributeUInt8", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();

        runs(function() {
            log("callGetAttributeUInt8");
            testInterfaceProxy.attributeUInt8.get().then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callGetAttributeUInt8", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            var retObj = spy.onFulfilled.calls[0].args[0];
            expect(retObj).toBeDefined();
            log("result = " + JSON.stringify(retObj));
            expect(retObj).toEqual(127);
        });
    });

    it("callSetAttributeDouble", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();

        runs(function() {
            log("callSetAttributeDouble");
            var args = {
                value: 1.1
            };
            testInterfaceProxy.attributeDouble.set(args).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callSetAttributeDouble", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
        });
    });

    it("callGetAttributeDouble", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();

        runs(function() {
            log("callGetAttributeDouble");
            testInterfaceProxy.attributeDouble.get().then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callGetAttributeDouble", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            var retObj = spy.onFulfilled.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(IltUtil.cmpDouble(retObj, 1.1)).toBeTruthy();
        });
    });

    it("callGetAttributeBooleanReadonly", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();

        runs(function() {
            log("callGetAttributeBooleanReadonly");
            testInterfaceProxy.attributeBooleanReadonly.get().then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callGetAttributeBooleanReadonly", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            var retObj = spy.onFulfilled.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj).toBeTruthy();
        });
    });

    it("callSetAttributeStringNoSubscriptions", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();

        runs(function() {
            log("callSetAttributeStringNoSubscriptions");
            var args = {
                value: "Hello world"
            };
            testInterfaceProxy.attributeStringNoSubscriptions.set(args).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callSetAttributeStringNoSubscriptions", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
        });
    });


    it("callGetAttributeStringNoSubscriptions", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();

        runs(function() {
            log("callGetAttributeStringNoSubscriptions");
            testInterfaceProxy.attributeStringNoSubscriptions.get().then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callGetAttributeStringNoSubscriptions", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            var retObj = spy.onFulfilled.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj).toEqual("Hello world");
        });
    });

    it("callGetAttributeInt8readonlyNoSubscriptions", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();

        runs(function() {
            log("callGetAttributeInt8readonlyNoSubscriptions");
            testInterfaceProxy.attributeInt8readonlyNoSubscriptions.get().then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callGetAttributeInt8readonlyNoSubscriptions", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            var retObj = spy.onFulfilled.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj).toEqual(-128);
        });
    });

    it("callSetAttributeArrayOfStringImplicit", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();

        runs(function() {
            log("callSetAttributeArrayOfStringImplicit");
            var args = {
                value: IltUtil.createStringArray()
            };
            testInterfaceProxy.attributeArrayOfStringImplicit.set(args).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callSetAttributeArrayOfStringImplicit", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
        });
    });

    it("callGetAttributeArrayOfStringImplicit", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();

        runs(function() {
            log("callGetAttributeArrayOfStringImplicit");
            testInterfaceProxy.attributeArrayOfStringImplicit.get().then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callGetAttributeArrayOfStringImplicit", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            var retObj = spy.onFulfilled.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(IltUtil.checkStringArray(retObj)).toBeTruthy();
        });
    });

    it("callSetAttributeEnumeration", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();

        runs(function() {
            log("callSetAttributeEnumeration");
            var args = {
                value: Enumeration.ENUM_0_VALUE_2
            };
            testInterfaceProxy.attributeEnumeration.set(args).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callSetAttributeEnumeration", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
        });
    });

    it("callGetAttributeEnumeration", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();

        runs(function() {
            log("callGetAttributeEnumeration");
            testInterfaceProxy.attributeEnumeration.get().then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callGetAttributeEnumeration", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            var retObj = spy.onFulfilled.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj).toEqual(Enumeration.ENUM_0_VALUE_2);
        });
    });

    it("callGetAttributeExtendedEnumerationReadonly", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();

        runs(function() {
            log("callGetAttributeExtendedEnumerationReadonly");
            testInterfaceProxy.attributeExtendedEnumerationReadonly.get().then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callGetAttributeExtendedEnumerationReadonly", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            var retObj = spy.onFulfilled.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj).toEqual(ExtendedEnumerationWithPartlyDefinedValues.ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES);
        });
    });

    it("callSetAttributeBaseStruct", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();

        runs(function() {
            log("callSetAttributeBaseStruct");
            var args = {
                value: IltUtil.createBaseStruct()
            };
            testInterfaceProxy.attributeBaseStruct.set(args).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callSetAttributeBaseStruct", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
        });
    });

    it("callGetAttributeBaseStruct", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();

        runs(function() {
            log("callGetAttributeBaseStruct");
            testInterfaceProxy.attributeBaseStruct.get().then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callGetAttributeBaseStruct", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            var retObj = spy.onFulfilled.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(IltUtil.checkBaseStruct(retObj)).toBeTruthy();
        });
    });

    it("callSetAttributeExtendedExtendedBaseStruct", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();

        runs(function() {
            log("callSetAttributeExtendedExtendedBaseStruct");
            var args = {
                value: IltUtil.createExtendedExtendedBaseStruct()
            };
            testInterfaceProxy.attributeExtendedExtendedBaseStruct.set(args).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callSetAttributeExtendedExtendedBaseStruct", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
        });
    });

    it("callGetAttributeExtendedExtendedBaseStruct", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
        spy.onFulfilled.reset();
        spy.onError.reset();

        runs(function() {
            log("callGetAttributeExtendedExtendedBaseStruct");
            testInterfaceProxy.attributeExtendedExtendedBaseStruct.get().then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callGetAttributeExtendedExtendedBaseStruct", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            var retObj = spy.onFulfilled.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(IltUtil.checkExtendedExtendedBaseStruct(retObj)).toBeTruthy();
        });
    });

    it("callSubscribeAttributeEnumeration", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError", "onPublication", "onPublicationError", "onSubscribed" ]);
        var subscriptionId;
        var subscriptionQosOnChange = new joynr.proxy.OnChangeSubscriptionQos({ minIntervalMs: 50, validityMs: 60000 });
        spy.onFulfilled.reset();
        spy.onError.reset();
        spy.onPublication.reset();
        spy.onPublicationError.reset();
        spy.onSubscribed.reset();

        runs(function() {
            log("callSubscribeAttributeEnumeration");
            testInterfaceProxy.attributeEnumeration.subscribe({
                "subscriptionQos": subscriptionQosOnChange,
                "onReceive": spy.onPublication,
                "onError": spy.onPublicationError,
                "onSubscribed": spy.onSubscribed
            }).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callSubscribeAttributeEnumeration", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            subscriptionId = spy.onFulfilled.calls[0].args[0];
            log("subscriptionId = " + subscriptionId);
        });

        waitsFor(function() {
            return spy.onSubscribed.callCount > 0;
        }, "callSubscribeAttributeEnumeration onSubscribed", 5000);

        runs(function() {
            expect(spy.onSubscribed.callCount).toEqual(1);
            expect(spy.onSubscribed.calls[0].args[0]).toEqual(subscriptionId);
        });

        waitsFor(function() {
            return spy.onPublication.callCount > 0 || spy.onPublicationError.callCount > 0;
        }, "callSubscribeAttributeEnumeration Publication", 5000);

        runs(function() {
            expect(spy.onPublication.callCount).toEqual(1);
            expect(spy.onPublicationError.callCount).toEqual(0);
            var retObj = spy.onPublication.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj).toEqual(Enumeration.ENUM_0_VALUE_2);

            // unsubscribe again
            spy.onFulfilled.reset();
            spy.onError.reset();
            testInterfaceProxy.attributeEnumeration.unsubscribe({
                "subscriptionId": subscriptionId
            }).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callSubscribeAttributeEnumeration unsubscribe", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
        });
    });

    it("callSubscribeAttributeWithExceptionFromGetter", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError", "onPublication", "onPublicationError", "onSubscribed" ]);
        var subscriptionId;
        var subscriptionQosOnChange = new joynr.proxy.OnChangeSubscriptionQos({ minIntervalMs: 50, validityMs: 60000 });
        spy.onFulfilled.reset();
        spy.onError.reset();
        spy.onPublication.reset();
        spy.onPublicationError.reset();
        spy.onSubscribed.reset();

        runs(function() {
            log("callSubscribeAttributeWithExceptionFromGetter");
            testInterfaceProxy.attributeWithExceptionFromGetter.subscribe({
                "subscriptionQos": subscriptionQosOnChange,
                "onReceive": spy.onPublication,
                "onError": spy.onPublicationError,
                "onSubscribed": spy.onSubscribed
            }).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callSubscribeAttributeWithExceptionFromGetter", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            subscriptionId = spy.onFulfilled.calls[0].args[0];
            log("callSubscribeAttributeWithExceptionFromGetter - subscriptionId = " + subscriptionId);
        });

        waitsFor(function() {
            return spy.onSubscribed.callCount > 0;
        }, "callSubscribeAttributeWithExceptionFromGetter onSubscribed", 5000);

        runs(function() {
            expect(spy.onSubscribed.callCount).toEqual(1);
            expect(spy.onSubscribed.calls[0].args[0]).toEqual(subscriptionId);
        });

        waitsFor(function() {
            return spy.onPublication.callCount > 0 || spy.onPublicationError.callCount > 0;
        }, "callSubscribeAttributeWithExceptionFromGetter Publication", 5000);

        runs(function() {
            expect(spy.onPublication.callCount).toEqual(0);
            expect(spy.onPublicationError.callCount).toEqual(1);
            var retObj = spy.onPublicationError.calls[0].args[0];
            log("retObj = " + JSON.stringify(retObj));
            expect(retObj).toBeDefined();
            expect(retObj._typeName).toEqual("joynr.exceptions.ProviderRuntimeException");
            expect(retObj.detailMessage).toBeDefined();
            expect(retObj.detailMessage).toEqual("Exception from getAttributeWithExceptionFromGetter");

            // unsubscribe again
            spy.onFulfilled.reset();
            spy.onError.reset();
            testInterfaceProxy.attributeWithExceptionFromGetter.unsubscribe({
                "subscriptionId": subscriptionId
            }).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callSubscribeAttributeWithExceptionFromGetter unsubscribe", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
        });
    });

    callSubscribeBroadcastWithSinglePrimitiveParameter = function(partitionsToUse) {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError", "onPublication", "onPublicationError", "onSubscribed" ]);
        var subscriptionId;
        var subscriptionQosOnChange = new joynr.proxy.OnChangeSubscriptionQos({ minIntervalMs: 50, validityMs: 60000 });
        var sleepDone;
        spy.onFulfilled.reset();
        spy.onError.reset();
        spy.onPublication.reset();
        spy.onPublicationError.reset();
        spy.onSubscribed.reset();

        runs(function() {
            log("callSubscribeBroadcastWithSinglePrimitiveParameter");
            testInterfaceProxy.broadcastWithSinglePrimitiveParameter.subscribe({
                "subscriptionQos": subscriptionQosOnChange,
                "partitions" : partitionsToUse,
                "onReceive": spy.onPublication,
                "onError": spy.onPublicationError,
                "onSubscribed": spy.onSubscribed
            }).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callSubscribeBroadcastWithSinglePrimitiveParameter", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            subscriptionId = spy.onFulfilled.calls[0].args[0];
            log("subscriptionId = " + subscriptionId);
        });

        waitsFor(function() {
            return spy.onSubscribed.callCount > 0;
        }, "callSubscribeBroadcastWithSinglePrimitiveParameter onSubscribed", 5000);

        runs(function() {
            expect(spy.onSubscribed.callCount).toEqual(1);
            expect(spy.onSubscribed.calls[0].args[0]).toEqual(subscriptionId);
            setTimeout(function() {
                sleepDone = true;
            }, 1000);
        });

        waitsFor(function() {
            return sleepDone;
        }, "callSubscribeBroadcastWithSinglePrimitiveParameter sleep done", 2000);

        runs(function() {
            // execute fire method here
            // note that it can take time, until the broadcast is registered
            // best if we would wait here for some time, and then fire the broadcast
            spy.onFulfilled.reset();
            spy.onError.reset();
            testInterfaceProxy.methodToFireBroadcastWithSinglePrimitiveParameter({
                partitions: partitionsToUse
            }).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callSubscribeBroadcastWithSinglePrimitiveParameter methodToFireBroadcastWithSinglePrimitiveParameter", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            // call to fire broadcast went ok
            // now wait for the publication to happen
        });

        waitsFor(function() {
            return spy.onPublication.callCount > 0 || spy.onPublicationError.callCount > 0;
        }, "callSubscribeBroadcastWithSinglePrimitiveParameter Publication", 5000);

        runs(function() {
            expect(spy.onPublication.callCount).toEqual(1);
            expect(spy.onPublicationError.callCount).toEqual(0);
            var retObj = spy.onPublication.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj.stringOut).toBeDefined();
            expect(retObj.stringOut).toEqual("boom");
            log("publication retObj: " + JSON.stringify(retObj));

            // unsubscribe again
            spy.onFulfilled.reset();
            spy.onError.reset();
            testInterfaceProxy.broadcastWithSinglePrimitiveParameter.unsubscribe({
                "subscriptionId": subscriptionId
            }).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callSubscribeBroadcastWithSinglePrimitiveParameter unsubscribe", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
        });
    }

    it("callSubscribeBroadcastWithSinglePrimitiveParameter_NoPartitions", function() {
        callSubscribeBroadcastWithSinglePrimitiveParameter([]);
    });

    it("callSubscribeBroadcastWithSinglePrimitiveParameter_SimplePartitions", function() {
        callSubscribeBroadcastWithSinglePrimitiveParameter(["partition0", "partition1"]);
    });

    callSubscribeBroadcastWithMultiplePrimitiveParameters = function(partitionsToUse) {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError", "onPublication", "onPublicationError", "onSubscribed" ]);
        var subscriptionId;
        var subscriptionQosOnChange = new joynr.proxy.OnChangeSubscriptionQos({ minIntervalMs: 50, validityMs: 60000 });
        spy.onFulfilled.reset();
        spy.onError.reset();
        spy.onPublication.reset();
        spy.onPublicationError.reset();
        spy.onSubscribed.reset();

        runs(function() {
            log("subscribeBroadcastWithMultiplePrimitiveParameters");
            testInterfaceProxy.broadcastWithMultiplePrimitiveParameters.subscribe({
                "subscriptionQos": subscriptionQosOnChange,
                "partitions" : partitionsToUse,
                "onReceive": spy.onPublication,
                "onError": spy.onPublicationError,
                "onSubscribed": spy.onSubscribed
            }).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "subscribeBroadcastWithMultiplePrimitiveParameters", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            subscriptionId = spy.onFulfilled.calls[0].args[0];
            log("subscriptionId = " + subscriptionId);
        });

        waitsFor(function() {
            return spy.onSubscribed.callCount > 0;
        }, "callSubscribeBroadcastWithMultiplePrimitiveParameters onSubscribed", 5000);

        runs(function() {
            expect(spy.onSubscribed.callCount).toEqual(1);
            expect(spy.onSubscribed.calls[0].args[0]).toEqual(subscriptionId);
            // execute fire method here
            spy.onFulfilled.reset();
            spy.onError.reset();
            testInterfaceProxy.methodToFireBroadcastWithMultiplePrimitiveParameters({
                partitions: partitionsToUse
            }).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "subscribeBroadcastWithMultiplePrimitiveParameters methodToFireBroadcastWithMultiplePrimitiveParameter", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            // call to fire broadcast went ok
            // now wait for the publication to happen
        });

        waitsFor(function() {
            return spy.onPublication.callCount > 0 || spy.onPublicationError.callCount > 0;
        }, "subscribeBroadcastWithMultiplePrimitiveParameters Publication", 5000);

        runs(function() {
            expect(spy.onPublication.callCount).toEqual(1);
            expect(spy.onPublicationError.callCount).toEqual(0);
            var retObj = spy.onPublication.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj.doubleOut).toBeDefined();
            expect(IltUtil.cmpDouble(retObj.doubleOut, 1.1)).toBeTruthy();
            expect(retObj.stringOut).toBeDefined();
            expect(retObj.stringOut).toEqual("boom");
            log("publication retObj: " + JSON.stringify(retObj));

            // unsubscribe again
            spy.onFulfilled.reset();
            spy.onError.reset();
            testInterfaceProxy.broadcastWithMultiplePrimitiveParameters.unsubscribe({
                "subscriptionId": subscriptionId
            }).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "subscribeBroadcastWithMultiplePrimitiveParameters unsubscribe", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
        });
    }

    it("callSubscribeBroadcastWithMultiplePrimitiveParameters_NoPartitions", function() {
        callSubscribeBroadcastWithMultiplePrimitiveParameters([]);
    });

    it("callSubscribeBroadcastWithMultiplePrimitiveParameters_SimplePartitions", function() {
        callSubscribeBroadcastWithMultiplePrimitiveParameters(["partition0", "partition1"]);
    });

    callSubscribeBroadcastWithSingleArrayParameter = function(partitionsToUse) {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError", "onPublication", "onPublicationError", "onSubscribed" ]);
        var subscriptionId;
        var subscriptionQosOnChange = new joynr.proxy.OnChangeSubscriptionQos({ minIntervalMs: 50, validityMs: 60000 });
        spy.onFulfilled.reset();
        spy.onError.reset();
        spy.onPublication.reset();
        spy.onPublicationError.reset();
        spy.onSubscribed.reset();

        runs(function() {
            log("callSubscribeBroadcastWithSingleArrayParameter");
            testInterfaceProxy.broadcastWithSingleArrayParameter.subscribe({
                "subscriptionQos": subscriptionQosOnChange,
                "partitions" : partitionsToUse,
                "onReceive": spy.onPublication,
                "onError": spy.onPublicationError,
                "onSubscribed": spy.onSubscribed
            }).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callSubscribeBroadcastWithSingleArrayParameter", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            subscriptionId = spy.onFulfilled.calls[0].args[0];
            log("subscriptionId = " + subscriptionId);
        });

        waitsFor(function() {
            return spy.onSubscribed.callCount > 0;
        }, "callSubscribeBroadcastWithSingleArrayParameter onSubscribed", 5000);

        runs(function() {
            expect(spy.onSubscribed.callCount).toEqual(1);
            expect(spy.onSubscribed.calls[0].args[0]).toEqual(subscriptionId);
            // execute fire method here
            spy.onFulfilled.reset();
            spy.onError.reset();
            testInterfaceProxy.methodToFireBroadcastWithSingleArrayParameter({
                partitions: partitionsToUse
            }).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callSubscribeBroadcastWithSingleArrayParameter methodToFireBroadcastWithSinglePrimitiveParameter", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            // call to fire broadcast went ok
            // now wait for the publication to happen
        });

        waitsFor(function() {
            return spy.onPublication.callCount > 0 || spy.onPublicationError.callCount > 0;
        }, "callSubscribeBroadcastWithSingleArrayParameter Publication", 5000);

        runs(function() {
            expect(spy.onPublication.callCount).toEqual(1);
            expect(spy.onPublicationError.callCount).toEqual(0);
            var retObj = spy.onPublication.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj.stringArrayOut).toBeDefined();
            expect(IltUtil.checkStringArray(retObj.stringArrayOut)).toBeTruthy();
            log("publication retObj: " + JSON.stringify(retObj));

            // unsubscribe again
            spy.onFulfilled.reset();
            spy.onError.reset();
            testInterfaceProxy.broadcastWithSingleArrayParameter.unsubscribe({
                "subscriptionId": subscriptionId
            }).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callSubscribeBroadcastWithSingleArrayParameter unsubscribe", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
        });
    }

    it("callSubscribeBroadcastWithSingleArrayParameter_NoPartitions", function() {
        callSubscribeBroadcastWithSingleArrayParameter([]);
    });

    it("callSubscribeBroadcastWithSingleArrayParameter_SimplePartitions", function() {
        callSubscribeBroadcastWithSingleArrayParameter(["partition0", "partition1"]);
    });

    callSubscribeBroadcastWithMultipleArrayParameters = function(partitionsToUse) {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError", "onPublication", "onPublicationError", "onSubscribed" ]);
        var subscriptionId;
        var subscriptionQosOnChange = new joynr.proxy.OnChangeSubscriptionQos({ minIntervalMs: 50, validityMs: 60000 });
        spy.onFulfilled.reset();
        spy.onError.reset();
        spy.onPublication.reset();
        spy.onPublicationError.reset();
        spy.onSubscribed.reset();

        runs(function() {
            log("callSubscribeBroadcastWithMultipleArrayParameters");
            testInterfaceProxy.broadcastWithMultipleArrayParameters.subscribe({
                "subscriptionQos": subscriptionQosOnChange,
                "partitions" : partitionsToUse,
                "onReceive": spy.onPublication,
                "onError": spy.onPublicationError,
                "onSubscribed": spy.onSubscribed
            }).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callSubscribeBroadcastWithMultipleArrayParameters", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            subscriptionId = spy.onFulfilled.calls[0].args[0];
            log("subscriptionId = " + subscriptionId);
            log("subscriptionId = " + subscriptionId);
        });

        waitsFor(function() {
            return spy.onSubscribed.callCount > 0;
        }, "callSubscribeBroadcastWithMultipleArrayParameters onSubscribed", 5000);

        runs(function() {
            expect(spy.onSubscribed.callCount).toEqual(1);
            expect(spy.onSubscribed.calls[0].args[0]).toEqual(subscriptionId);
            // execute fire method here
            spy.onFulfilled.reset();
            spy.onError.reset();
            testInterfaceProxy.methodToFireBroadcastWithMultipleArrayParameters({
                partitions: partitionsToUse
            }).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callSubscribeBroadcastWithMultipleArrayParameters methodToFireBroadcastWithSinglePrimitiveParameter", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            // call to fire broadcast went ok
            // now wait for the publication to happen
        });

        waitsFor(function() {
            return spy.onPublication.callCount > 0 || spy.onPublicationError.callCount > 0;
        }, "callSubscribeBroadcastWithMultipleArrayParameters Publication", 5000);

        runs(function() {
            expect(spy.onPublication.callCount).toEqual(1);
            expect(spy.onPublicationError.callCount).toEqual(0);
            var retObj = spy.onPublication.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj.uInt64ArrayOut).toBeDefined();
            expect(IltUtil.checkUInt64Array(retObj.uInt64ArrayOut)).toBeTruthy();
            expect(retObj.structWithStringArrayArrayOut).toBeDefined();
            expect(IltUtil.checkStructWithStringArrayArray(retObj.structWithStringArrayArrayOut)).toBeTruthy();
            log("publication retObj: " + JSON.stringify(retObj));

            // unsubscribe again
            spy.onFulfilled.reset();
            spy.onError.reset();
            testInterfaceProxy.broadcastWithMultipleArrayParameters.unsubscribe({
                "subscriptionId": subscriptionId
            }).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callSubscribeBroadcastWithMultipleArrayParameters unsubscribe", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
        });
    }

    it("callSubscribeBroadcastWithMultipleArrayParameters_NoPartitions", function() {
        callSubscribeBroadcastWithMultipleArrayParameters([]);
    });

    it("callSubscribeBroadcastWithMultipleArrayParameters_SimplePartitions", function() {
        callSubscribeBroadcastWithMultipleArrayParameters(["partition0", "partition1"]);
    });

    callSubscribeBroadcastWithSingleEnumerationParameter = function(partitionsToUse) {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError", "onPublication", "onPublicationError", "onSubscribed" ]);
        var subscriptionId;
        var subscriptionQosOnChange = new joynr.proxy.OnChangeSubscriptionQos({ minIntervalMs: 50, validityMs: 60000 });
        spy.onFulfilled.reset();
        spy.onError.reset();
        spy.onPublication.reset();
        spy.onPublicationError.reset();
        spy.onSubscribed.reset();

        runs(function() {
            log("callSubscribeBroadcastWithSingleEnumerationParameter");
            testInterfaceProxy.broadcastWithSingleEnumerationParameter.subscribe({
                "subscriptionQos": subscriptionQosOnChange,
                "partitions" : partitionsToUse,
                "onReceive": spy.onPublication,
                "onError": spy.onPublicationError,
                "onSubscribed": spy.onSubscribed
            }).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callSubscribeBroadcastWithSingleEnumerationParameter", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            subscriptionId = spy.onFulfilled.calls[0].args[0];
            log("subscriptionId = " + subscriptionId);
        });

        waitsFor(function() {
            return spy.onSubscribed.callCount > 0;
        }, "callSubscribeBroadcastWithSingleEnumerationParameter onSubscribed", 5000);

        runs(function() {
            expect(spy.onSubscribed.callCount).toEqual(1);
            expect(spy.onSubscribed.calls[0].args[0]).toEqual(subscriptionId);
            // execute fire method here
            spy.onFulfilled.reset();
            spy.onError.reset();
            testInterfaceProxy.methodToFireBroadcastWithSingleEnumerationParameter({
                partitions: partitionsToUse
            }).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callSubscribeBroadcastWithSingleEnumerationParameter methodToFireBroadcastWithSinglePrimitiveParameter", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            // call to fire broadcast went ok
            // now wait for the publication to happen
        });

        waitsFor(function() {
            return spy.onPublication.callCount > 0 || spy.onPublicationError.callCount > 0;
        }, "callSubscribeBroadcastWithSingleEnumerationParameter Publication", 5000);

        runs(function() {
            expect(spy.onPublication.callCount).toEqual(1);
            expect(spy.onPublicationError.callCount).toEqual(0);
            var retObj = spy.onPublication.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj.enumerationOut).toBeDefined();
            expect(retObj.enumerationOut).toEqual(ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION);
            log("publication retObj: " + JSON.stringify(retObj));

            // unsubscribe again
            spy.onFulfilled.reset();
            spy.onError.reset();
            testInterfaceProxy.broadcastWithSingleEnumerationParameter.unsubscribe({
                "subscriptionId": subscriptionId
            }).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callSubscribeBroadcastWithSingleEnumerationParameter unsubscribe", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
        });
    }

    it("callSubscribeBroadcastWithSingleEnumerationParameter_NoPartitions", function() {
        callSubscribeBroadcastWithSingleEnumerationParameter([]);
    });

    it("callSubscribeBroadcastWithSingleEnumerationParameter_SimplePartitions", function() {
        callSubscribeBroadcastWithSingleEnumerationParameter(["partition0", "partition1"]);
    });

    callSubscribeBroadcastWithMultipleEnumerationParameter = function(partitionsToUse) {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError", "onPublication", "onPublicationError", "onSubscribed" ]);
        var subscriptionId;
        var subscriptionQosOnChange = new joynr.proxy.OnChangeSubscriptionQos({ minIntervalMs: 50, validityMs: 60000 });
        spy.onFulfilled.reset();
        spy.onError.reset();
        spy.onPublication.reset();
        spy.onPublicationError.reset();
        spy.onSubscribed.reset();

        runs(function() {
            log("callSubscribeBroadcastWithMultipleEnumerationParameters");
            testInterfaceProxy.broadcastWithMultipleEnumerationParameters.subscribe({
                "subscriptionQos": subscriptionQosOnChange,
                "partitions" : partitionsToUse,
                "onReceive": spy.onPublication,
                "onError": spy.onPublicationError,
                "onSubscribed": spy.onSubscribed
            }).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callSubscribeBroadcastWithMultipleEnumerationParameters", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            subscriptionId = spy.onFulfilled.calls[0].args[0];
            log("subscriptionId = " + subscriptionId);
            setTimeout(function() {
                sleepDone = true;
            }, 1000);
        });

        waitsFor(function() {
            return spy.onSubscribed.callCount > 0;
        }, "callSubscribeBroadcastWithMultipleEnumerationParameters onSubscribed", 5000);

        runs(function() {
            expect(spy.onSubscribed.callCount).toEqual(1);
            expect(spy.onSubscribed.calls[0].args[0]).toEqual(subscriptionId);
            // execute fire method here
            spy.onFulfilled.reset();
            spy.onError.reset();
            testInterfaceProxy.methodToFireBroadcastWithMultipleEnumerationParameters({
                partitions: partitionsToUse
            }).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callSubscribeBroadcastWithMultipleEnumerationParameters methodToFireBroadcastWithSinglePrimitiveParameter", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            // call to fire broadcast went ok
            // now wait for the publication to happen
        });

        waitsFor(function() {
            return spy.onPublication.callCount > 0 || spy.onPublicationError.callCount > 0;
        }, "callSubscribeBroadcastWithMultipleEnumerationParameters Publication", 5000);

        runs(function() {
            expect(spy.onPublication.callCount).toEqual(1);
            expect(spy.onPublicationError.callCount).toEqual(0);
            var retObj = spy.onPublication.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj.extendedEnumerationOut).toBeDefined();
            expect(retObj.extendedEnumerationOut).toEqual(ExtendedEnumerationWithPartlyDefinedValues.ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES);
            expect(retObj.enumerationOut).toBeDefined();
            expect(retObj.enumerationOut).toEqual(Enumeration.ENUM_0_VALUE_1);

            // unsubscribe again
            spy.onFulfilled.reset();
            spy.onError.reset();
            testInterfaceProxy.broadcastWithMultipleEnumerationParameters.unsubscribe({
                "subscriptionId": subscriptionId
            }).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callSubscribeBroadcastWithMultipleEnumerationParameters unsubscribe", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
        });
    }

    it("callSubscribeBroadcastWithMultipleEnumerationParameter_NoPartitions", function() {
        callSubscribeBroadcastWithMultipleEnumerationParameter([]);
    });

    it("callSubscribeBroadcastWithMultipleEnumerationParameter_SimplePartitions", function() {
        callSubscribeBroadcastWithMultipleEnumerationParameter(["partition0", "partition1"]);
    });

    callSubscribeBroadcastWithSingleStructParameter = function(partitionsToUse) {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError", "onPublication", "onPublicationError", "onSubscribed" ]);
        var subscriptionId;
        var subscriptionQosOnChange = new joynr.proxy.OnChangeSubscriptionQos({ minIntervalMs: 50, validityMs: 60000 });
        spy.onFulfilled.reset();
        spy.onError.reset();
        spy.onPublication.reset();
        spy.onPublicationError.reset();
        spy.onSubscribed.reset();

        runs(function() {
            log("callSubscribeBroadcastWithSingleStructParameter");
            testInterfaceProxy.broadcastWithSingleStructParameter.subscribe({
                "subscriptionQos": subscriptionQosOnChange,
                "partitions" : partitionsToUse,
                "onReceive": spy.onPublication,
                "onError": spy.onPublicationError,
                "onSubscribed": spy.onSubscribed
            }).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callSubscribeBroadcastWithSingleStructParameter", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            subscriptionId = spy.onFulfilled.calls[0].args[0];
            log("subscriptionId = " + subscriptionId);
        });

        waitsFor(function() {
            return spy.onSubscribed.callCount > 0;
        }, "callSubscribeBroadcastWithSingleStructParameter onSubscribed", 5000);

        runs(function() {
            expect(spy.onSubscribed.callCount).toEqual(1);
            expect(spy.onSubscribed.calls[0].args[0]).toEqual(subscriptionId);
        });

        runs(function() {
            // execute fire method here
            // note that it can take time, until the broadcast is registered
            // best if we would wait here for some time, and then fire the broadcast
            spy.onFulfilled.reset();
            spy.onError.reset();
            testInterfaceProxy.methodToFireBroadcastWithSingleStructParameter({
                partitions: partitionsToUse
            }).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callSubscribeBroadcastWithSingleStructParameter methodToFireBroadcastWithSinglePrimitiveParameter", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            // call to fire broadcast went ok
            // now wait for the publication to happen
        });

        waitsFor(function() {
            return spy.onPublication.callCount > 0 || spy.onPublicationError.callCount > 0;
        }, "callSubscribeBroadcastWithSingleStructParameter Publication", 5000);

        runs(function() {
            expect(spy.onPublication.callCount).toEqual(1);
            expect(spy.onPublicationError.callCount).toEqual(0);
            var retObj = spy.onPublication.calls[0].args[0];
            expect(retObj).toBeDefined();
            expect(retObj.extendedStructOfPrimitivesOut).toBeDefined();
            expect(IltUtil.checkExtendedStructOfPrimitives(retObj.extendedStructOfPrimitivesOut)).toBeTruthy();
            log("publication retObj: " + JSON.stringify(retObj));

            // unsubscribe again
            spy.onFulfilled.reset();
            spy.onError.reset();
            testInterfaceProxy.broadcastWithSingleStructParameter.unsubscribe({
                "subscriptionId": subscriptionId
            }).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callSubscribeBroadcastWithSingleStructParameter unsubscribe", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
        });
    }

    it("callSubscribeBroadcastWithSingleStructParameter_NoPartitions", function() {
        callSubscribeBroadcastWithSingleStructParameter([]);
    });

    it("callSubscribeBroadcastWithSingleStructParameter_SimplePartitions", function() {
        callSubscribeBroadcastWithSingleStructParameter(["partition0", "partition1"]);
    });

    callSubscribeBroadcastWithMultipleStructParameter = function(partitionsToUse) {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError", "onPublication", "onPublicationError", "onSubscribed" ]);
        var subscriptionId;
        var subscriptionQosOnChange = new joynr.proxy.OnChangeSubscriptionQos({ minIntervalMs: 50, validityMs: 60000 });
        spy.onFulfilled.reset();
        spy.onError.reset();
        spy.onPublication.reset();
        spy.onPublicationError.reset();

        runs(function() {
            log("callSubscribeBroadcastWithMultipleStructParameters");
            testInterfaceProxy.broadcastWithMultipleStructParameters.subscribe({
                "subscriptionQos": subscriptionQosOnChange,
                "partitions" : partitionsToUse,
                "onReceive": spy.onPublication,
                "onError": spy.onPublicationError,
                "onSubscribed": spy.onSubscribed
            }).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callSubscribeBroadcastWithMultipleStructParameters", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            subscriptionId = spy.onFulfilled.calls[0].args[0];
            log("subscriptionId = " + subscriptionId);
        });

        waitsFor(function() {
            return spy.onSubscribed.callCount > 0;
        }, "callSubscribeBroadcastWithMultipleStructParameters onSubscribed", 5000);

        runs(function() {
            expect(spy.onSubscribed.callCount).toEqual(1);
            expect(spy.onSubscribed.calls[0].args[0]).toEqual(subscriptionId);
            // execute fire method here
            spy.onFulfilled.reset();
            spy.onError.reset();
            testInterfaceProxy.methodToFireBroadcastWithMultipleStructParameters({
                partitions: partitionsToUse
            }).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callSubscribeBroadcastWithMultipleStructParameters methodToFireBroadcastWithMultipleStructParameters", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            // call to fire broadcast went ok
            // now wait for the publication to happen
        });

        waitsFor(function() {
            return spy.onPublication.callCount > 0 || spy.onPublicationError.callCount > 0;
        }, "callSubscribeBroadcastWithMultipleStructParameters Publication", 5000);

        runs(function() {
            expect(spy.onPublication.callCount).toEqual(1);
            expect(spy.onPublicationError.callCount).toEqual(0);
            var retObj = spy.onPublication.calls[0].args[0];
            log("XXX: publication retObj: " + JSON.stringify(retObj));
            expect(retObj).toBeDefined();
            expect(retObj.baseStructWithoutElementsOut).toBeDefined();
            expect(IltUtil.checkBaseStructWithoutElements(retObj.baseStructWithoutElementsOut)).toBeTruthy();
            expect(retObj.extendedExtendedBaseStructOut).toBeDefined();
            expect(IltUtil.checkExtendedExtendedBaseStruct(retObj.extendedExtendedBaseStructOut)).toBeTruthy();

            // unsubscribe again
            spy.onFulfilled.reset();
            spy.onError.reset();
            testInterfaceProxy.broadcastWithMultipleStructParameters.unsubscribe({
                "subscriptionId": subscriptionId
            }).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callSubscribeBroadcastWithMultipleStructParameters unsubscribe", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
        });
    }

    it("callSubscribeBroadcastWithMultipleStructParameter_NoPartitions", function() {
        callSubscribeBroadcastWithMultipleStructParameter([]);
    });

    it("callSubscribeBroadcastWithMultipleStructParameter_SimplePartitions", function() {
        callSubscribeBroadcastWithMultipleStructParameter(["partition0", "partition1"]);
    });

    it("doNotReceivePublicationsForOtherPartitions", function() {
        var spy = jasmine.createSpyObj("spy",
            [ "onFulfilled", "onError", "onPublication", "onPublicationError", "onSubscribed" ]);
        var subscriptionId;
        var subscriptionQosOnChange = new joynr.proxy.OnChangeSubscriptionQos({ minIntervalMs: 50, validityMs: 60000 });

        var subscribeToPartitions = ["partition0", "partition1"];
        var broadcastPartition = ["otherPartition"];

        runs(function() {
            log("doNotReceivePublicationsForOtherPartitions");
            testInterfaceProxy.broadcastWithSingleEnumerationParameter.subscribe({
                "subscriptionQos": subscriptionQosOnChange,
                "partitions" : subscribeToPartitions,
                "onReceive": spy.onPublication,
                "onError": spy.onPublicationError,
                "onSubscribed": spy.onSubscribed
            }).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "doNotReceivePublicationsForOtherPartitions waitForSubscribe", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            subscriptionId = spy.onFulfilled.calls[0].args[0];
            log("subscriptionId = " + subscriptionId);
        });

        waitsFor(function() {
            return spy.onSubscribed.callCount > 0;
        }, "doNotReceivePublicationsForOtherPartitions onSubscribed", 5000);

        runs(function() {
            spy.onFulfilled.reset();
            spy.onError.reset();
            spy.onPublication.reset();
            spy.onPublicationError.reset();
            testInterfaceProxy.methodToFireBroadcastWithSingleEnumerationParameter({
                partitions: broadcastPartition
            }).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0 ||
                   spy.onPublication.callCount > 0 || spy.onPublicationError.callCount > 0;
        }, "doNotReceivePublicationsForOtherPartitions receiveNoPublication", 2000);

        runs(function() {
            // The partitions do not match. Expect no broadcast
            expect(spy.onPublication.callCount).toEqual(0);
            expect(spy.onPublicationError.callCount).toEqual(0);
        });

        runs(function() {
            spy.onFulfilled.reset();
            spy.onError.reset();
            spy.onPublication.reset();
            spy.onPublicationError.reset();
            // Make sure there is no other reason that we did not receive any broadcast
            testInterfaceProxy.methodToFireBroadcastWithSingleEnumerationParameter({
                partitions: subscribeToPartitions
            }).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onPublication.callCount > 0 || spy.onPublicationError.callCount;
        }, "doNotReceivePublicationsForOtherPartitions receivePublication", 2000);

        waitsFor(function() {
            // Wait until testInterfaceProxy.methodToFireBroadcastWithSingleEnumerationParameter finished
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "doNotReceivePublicationsForOtherPartitions fireBroadcastFinished", 5000);

        runs(function() {
            expect(spy.onPublication.callCount).toEqual(1);
            expect(spy.onPublicationError.callCount).toEqual(0);
        });

        runs(function() {
            // unsubscribe
            spy.onFulfilled.reset();
            spy.onError.reset();
            testInterfaceProxy.broadcastWithMultipleStructParameters.unsubscribe({
                "subscriptionId": subscriptionId
            }).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "doNotReceivePublicationsForOtherPartitions unsubscribe", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
        });
    });

    it("callSubscribeBroadcastWithFiltering", function() {
        var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError", "onPublication", "onPublicationError", "onSubscribed" ]);
        var subscriptionId;
        var subscriptionQosOnChange = new joynr.proxy.OnChangeSubscriptionQos({ minIntervalMs: 50, validityMs: 60000 });
        spy.onFulfilled.reset();
        spy.onError.reset();
        spy.onPublication.reset();
        spy.onPublicationError.reset();
        spy.onSubscribed.reset();

        runs(function() {
            log("callSubscribeBroadcastWithFiltering");
            var filterParameters = testInterfaceProxy.broadcastWithFiltering.createFilterParameters();
            var stringOfInterest = "fireBroadcast";
            filterParameters.setStringOfInterest(stringOfInterest);
            //filterParameters.setStringArrayOfInterest(JSON.stringify(IltUtil.createStringArray()));
            //filterParameters.setEnumerationOfInterest(JSON.stringify(ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION));
            //filterParameters.setStructWithStringArrayOfInterest(JSON.stringify(IltUtil.createStructWithStringArray()));
            //filterParameters.setStructWithStringArrayArrayOfInterest(JSON.stringify(IltUtil.createStructWithStringArrayArray()));
            filterParameters.setStringArrayOfInterest("[\"Hello\",\"World\"]");
            filterParameters.setEnumerationOfInterest("\"ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION\"");
            filterParameters.setStructWithStringArrayOfInterest("{\"_typeName\":\"joynr.interlanguagetest.namedTypeCollection1.StructWithStringArray\",\"stringArrayElement\":[\"Hello\",\"World\"]}");
            filterParameters.setStructWithStringArrayArrayOfInterest("[{\"_typeName\":\"joynr.interlanguagetest.namedTypeCollection1.StructWithStringArray\",\"stringArrayElement\":[\"Hello\",\"World\"]},{\"_typeName\":\"joynr.interlanguagetest.namedTypeCollection1.StructWithStringArray\",\"stringArrayElement\":[\"Hello\",\"World\"]}]");

            testInterfaceProxy.broadcastWithFiltering.subscribe({
                "subscriptionQos": subscriptionQosOnChange,
                "onReceive": spy.onPublication,
                "onError": spy.onPublicationError,
                "onSubscribed": spy.onSubscribed,
                "filterParameters": filterParameters
            }).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callSubscribeBroadcastWithFiltering", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            subscriptionId = spy.onFulfilled.calls[0].args[0];
            log("subscriptionId = " + subscriptionId);
        });

        waitsFor(function() {
            return spy.onSubscribed.callCount > 0;
        }, "callSubscribeBroadcastWithFiltering onSubscribed", 5000);

        runs(function() {
            expect(spy.onSubscribed.callCount).toEqual(1);
            expect(spy.onSubscribed.calls[0].args[0]).toEqual(subscriptionId);
            // execute fire method here
            spy.onFulfilled.reset();
            spy.onError.reset();
            var args = {
                stringArg: "fireBroadcast"
            };
            testInterfaceProxy.methodToFireBroadcastWithFiltering(args).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callSubscribeBroadcastWithFiltering methodToFireBroadcastWithSinglePrimitiveParameter", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
            // call to fire broadcast went ok
            // now wait for the publication to happen
        });

        waitsFor(function() {
            return spy.onPublication.callCount > 0 || spy.onPublicationError.callCount > 0;
        }, "callSubscribeBroadcastWithFiltering Publication", 5000);

        runs(function() {
            expect(spy.onPublication.callCount).toEqual(1);
            expect(spy.onPublicationError.callCount).toEqual(0);
            var retObj = spy.onPublication.calls[0].args[0];
            log("XXX: publication retObj: " + JSON.stringify(retObj));
            expect(retObj).toBeDefined();
            expect(retObj.stringOut).toBeDefined();
            expect(retObj.stringOut).toEqual("fireBroadcast");
            expect(retObj.enumerationOut).toBeDefined();
            expect(retObj.enumerationOut).toEqual(ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION);
            expect(retObj.stringArrayOut).toBeDefined();
            expect(IltUtil.checkStringArray(retObj.stringArrayOut)).toBeTruthy();
            expect(retObj.structWithStringArrayOut).toBeDefined();
            expect(IltUtil.checkStructWithStringArray(retObj.structWithStringArrayOut)).toBeTruthy();
            expect(retObj.structWithStringArrayArrayOut).toBeDefined();
            expect(IltUtil.checkStructWithStringArrayArray(retObj.structWithStringArrayArrayOut)).toBeTruthy();

            // unsubscribe again
            spy.onFulfilled.reset();
            spy.onError.reset();
            testInterfaceProxy.broadcastWithFiltering.unsubscribe({
                "subscriptionId": subscriptionId
            }).then(spy.onFulfilled).catch(spy.onError);
        });

        waitsFor(function() {
            return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
        }, "callSubscribeBroadcastWithFiltering unsubscribe", 5000);

        runs(function() {
            if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                log(spy.onError.calls[0].args[0]);
            }
            expect(spy.onFulfilled.callCount).toEqual(1);
            expect(spy.onError.callCount).toEqual(0);
        });
    });

    it("has finished the tests", function() {
        testFinished = true;
    });

    afterEach(function() {
        if (testFinished === true) {
            joynr.shutdown();
        }
    });
});
