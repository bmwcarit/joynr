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

const Promise = require("../../../main/js/global/Promise");
let TestEnd2EndDatatypesTestData = require("./TestEnd2EndDatatypesTestData"),
    IntegrationUtils = require("./IntegrationUtils"),
    provisioning = require("../../resources/joynr/provisioning/provisioning_cc"),
    End2EndAbstractTest = require("./End2EndAbstractTest"),
    waitsFor = require("../global/WaitsFor");
describe("libjoynr-js.integration.end2end.datatypes", () => {
    let datatypesProxy;
    const abstractTest = new End2EndAbstractTest("End2EndDatatypesTest", "TestEnd2EndDatatypesProviderProcess");

    beforeEach(done => {
        abstractTest.beforeEach().then(settings => {
            datatypesProxy = settings.dataProxy;
            done();
        });
    });

    it(
        "supports all datatypes in attributes get/set",
        done => {
            let i, j;

            function testAttrType(attributeName, attributeValue) {
                return new Promise((resolve, reject) => {
                    let attribute;
                    attribute = datatypesProxy[attributeName];
                    attribute
                        .set({
                            value: attributeValue
                        })
                        .then(() => {
                            // get the value
                            attribute.get().then(resolve, reject);
                            return null;
                        })
                        .catch(error => {
                            reject(error);
                            IntegrationUtils.outputPromiseError(error);
                        });
                });
            }

            function setAndGetAttribute(attributeName, attributeValue, promiseChain) {
                return promiseChain.then(() => {
                    return testAttrType(attributeName, attributeValue)
                        .then(value => {
                            expect(value).toEqual(attributeValue);
                            IntegrationUtils.checkValueAndType(value, attributeValue);
                        })
                        .catch(IntegrationUtils.outputPromiseError);
                });
            }

            let promiseChain = Promise.resolve();
            for (i = 0; i < TestEnd2EndDatatypesTestData.length; ++i) {
                const test = TestEnd2EndDatatypesTestData[i];
                for (j = 0; j < test.values.length; ++j) {
                    promiseChain = setAndGetAttribute(test.attribute, test.values[j], promiseChain);
                }
            }
            promiseChain
                .then(() => {
                    done();
                    return null;
                })
                .catch(fail);
        },
        120000
    );

    it(
        "supports all datatypes as operation arguments",
        done => {
            let i;

            function testGetJavascriptType(arg, expectedReturnValue, promiseChain) {
                return promiseChain.then(() => {
                    let onFulfilledSpy;
                    onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");
                    datatypesProxy
                        .getJavascriptType({
                            arg
                        })
                        .then(onFulfilledSpy)
                        .catch(IntegrationUtils.outputPromiseError);

                    return waitsFor(
                        () => {
                            return onFulfilledSpy.calls.count() > 0;
                        },
                        "operation is called",
                        provisioning.ttl
                    ).then(() => {
                        expect(onFulfilledSpy).toHaveBeenCalled();
                        expect(onFulfilledSpy).toHaveBeenCalledWith({
                            javascriptType: expectedReturnValue
                        });
                    });
                });
            }

            let promiseChain = Promise.resolve();
            for (i = 0; i < TestEnd2EndDatatypesTestData.length; ++i) {
                const test = TestEnd2EndDatatypesTestData[i];
                promiseChain = testGetJavascriptType(test.values[0], test.jsRuntimeType, promiseChain);
            }
            promiseChain
                .then(() => {
                    done();
                    return null;
                })
                .catch(fail);
        },
        60000
    );

    it(
        "supports all datatypes as operation argument and return value",
        done => {
            let i;

            function testGetArgumentBack(arg, promiseChain) {
                return promiseChain.then(() => {
                    return datatypesProxy
                        .getArgumentBack({
                            arg
                        })
                        .then(value => {
                            expect(value).toEqual({ returnValue: arg });
                            IntegrationUtils.checkValueAndType(value.returnValue, arg);
                        })
                        .catch(IntegrationUtils.outputPromiseError);
                });
            }

            let promiseChain = Promise.resolve();
            for (i = 0; i < TestEnd2EndDatatypesTestData.length; ++i) {
                const test = TestEnd2EndDatatypesTestData[i];
                promiseChain = testGetArgumentBack(test.values[0], promiseChain);
            }
            promiseChain
                .then(() => {
                    done();
                    return null;
                })
                .catch(fail);
        },
        60000
    );

    it(
        "supports multiple operation arguments",
        done => {
            let i;

            function testMultipleArguments(opArgs) {
                let onFulfilledSpy;

                onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");
                datatypesProxy
                    .multipleArguments(opArgs)
                    .then(onFulfilledSpy)
                    .catch(IntegrationUtils.outputPromiseError);

                return waitsFor(
                    () => {
                        return onFulfilledSpy.calls.count() > 0;
                    },
                    "operation is called",
                    provisioning.ttl
                ).then(() => {
                    expect(onFulfilledSpy).toHaveBeenCalled();
                    expect(onFulfilledSpy).toHaveBeenCalledWith({
                        serialized: JSON.stringify(opArgs)
                    });
                });
            }

            const opArgs = {};
            for (i = 0; i < TestEnd2EndDatatypesTestData.length; ++i) {
                const test = TestEnd2EndDatatypesTestData[i];
                /* replace all dots with _ */
                let paramName = test.joynrType.replace(/\./g, "_") + "Arg";
                paramName = paramName.slice(0, 1).toLowerCase() + paramName.slice(1);
                opArgs[paramName] = test.values[0];
            }
            testMultipleArguments(opArgs)
                .then(() => {
                    done();
                    return null;
                })
                .catch(fail);
        },
        60000
    );

    afterEach(abstractTest.afterEach);
});
