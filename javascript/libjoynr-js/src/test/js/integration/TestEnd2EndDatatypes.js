/*global joynrTestRequire: true */
/*jslint es5: true */

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

joynrTestRequire("integration/TestEnd2EndDatatypes", [
    "global/Promise",
    "joynr",
    "joynr/datatypes/DatatypesProxy",
    "joynr/datatypes/exampletypes/Country",
    "integration/TestEnd2EndDatatypesTestData",
    "integration/IntegrationUtils",
    "joynr/provisioning/provisioning_cc"
], function(
        Promise,
        joynr,
        DatatypesProxy,
        Country,
        TestEnd2EndDatatypesTestData,
        IntegrationUtils,
        provisioning) {
    describe("libjoynr-js.integration.end2end.datatypes", function() {

        var datatypesProxy, provisioningSuffix, workerId;

        beforeEach(function() {
            var testProvisioning = null;
            datatypesProxy = undefined;
            provisioningSuffix = "-" + Date.now();
            testProvisioning = IntegrationUtils.getProvisioning(provisioning, provisioningSuffix);
            runs(function() {
                joynr.load(testProvisioning, function(error, newJoynr) {
                    if (error) {
                        throw error;
                    }

                    joynr = newJoynr;
                    IntegrationUtils.initialize(joynr);

                    IntegrationUtils.initializeWebWorker(
                            "TestEnd2EndDatatypesProviderWorker",
                            provisioningSuffix).then(function(newWorkerId) {
                        workerId = newWorkerId;
                        return IntegrationUtils.startWebWorker(workerId);
                    }).then(
                            function() {
                                return IntegrationUtils.buildProxy(DatatypesProxy).then(
                                        function(newDatatypesProxy) {
                                            datatypesProxy = newDatatypesProxy;
                                        });
                            });
                });
            });
            waitsFor(function() {
                return datatypesProxy !== undefined;
            }, "proxy to be resolved", testProvisioning.ttl);
        });

        it("supports all datatypes in attributes get/set", function() {
            var i, j;

            function testAttrType(attributeName, attributeValue) {
                return new Promise(function(resolve, reject) {

                    var attribute;
                    attribute = datatypesProxy[attributeName];
                    attribute.set({
                        value : attributeValue
                    }).then(function() {
                        // get the value
                        attribute.get().then(resolve, reject);
                    }).catch(function(error) {
                        reject(error);
                        IntegrationUtils.outputPromiseError(error);
                    });
                });
            }

            function setAndGetAttribute(attributeName, attributeValue) {
                var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");

                runs(function() {
                    testAttrType(attributeName, attributeValue).then(
                            onFulfilledSpy).catch(IntegrationUtils.outputPromiseError);
                });

                waitsFor(function() {
                    return onFulfilledSpy.callCount > 0;
                }, "attribute set/get", 2 * provisioning.ttl);

                runs(function() {
                    expect(onFulfilledSpy).toHaveBeenCalled();
                    expect(onFulfilledSpy).toHaveBeenCalledWith(attributeValue);
                    IntegrationUtils.checkValueAndType(
                            onFulfilledSpy.calls[0].args[0],
                            attributeValue);
                });
            }

            for (i = 0; i < TestEnd2EndDatatypesTestData.length; ++i) {
                var test = TestEnd2EndDatatypesTestData[i];
                for (j = 0; j < test.values.length; ++j) {
                    setAndGetAttribute(test.attribute, test.values[j]);
                }
            }

        });

        it("supports all datatypes as operation arguments", function() {
            var i;

            function testGetJavascriptType(arg, expectedReturnValue) {
                var onFulfilledSpy;

                runs(function() {
                    onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");
                    datatypesProxy.getJavascriptType({
                        arg : arg
                    }).then(onFulfilledSpy).catch(IntegrationUtils.outputPromiseError);
                });

                waitsFor(function() {
                    return onFulfilledSpy.callCount > 0;
                }, "operation is called", provisioning.ttl);

                runs(function() {
                    expect(onFulfilledSpy).toHaveBeenCalled();
                    expect(onFulfilledSpy).toHaveBeenCalledWith(expectedReturnValue);
                });
            }

            for (i = 0; i < TestEnd2EndDatatypesTestData.length; ++i) {
                var test = TestEnd2EndDatatypesTestData[i];
                testGetJavascriptType(test.values[0], test.jsRuntimeType);
            }
        });

        it("supports all datatypes as operation argument and return value", function() {
            var i;

            function testGetArgumentBack(arg) {
                var onFulfilledSpy;

                runs(function() {
                    onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");
                    datatypesProxy.getArgumentBack({
                        arg : arg
                    }).then(onFulfilledSpy).catch(IntegrationUtils.outputPromiseError);
                });

                waitsFor(function() {
                    return onFulfilledSpy.callCount > 0;
                }, "operation is called", provisioning.ttl);

                runs(function() {
                    expect(onFulfilledSpy).toHaveBeenCalled();
                    expect(onFulfilledSpy).toHaveBeenCalledWith(arg);
                    IntegrationUtils.checkValueAndType(onFulfilledSpy.calls[0].args[0], arg);
                });
            }

            for (i = 0; i < TestEnd2EndDatatypesTestData.length; ++i) {
                var test = TestEnd2EndDatatypesTestData[i];
                testGetArgumentBack(test.values[0]);
            }
        });

        it("supports multiple operation arguments", function() {
            var i;

            function testMultipleArguments(opArgs) {
                var onFulfilledSpy;

                runs(function() {
                    onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");
                    datatypesProxy.multipleArguments(opArgs).then(
                            onFulfilledSpy).catch(IntegrationUtils.outputPromiseError);
                });

                waitsFor(function() {
                    return onFulfilledSpy.callCount > 0;
                }, "operation is called", provisioning.ttl);

                runs(function() {
                    expect(onFulfilledSpy).toHaveBeenCalled();
                    expect(onFulfilledSpy).toHaveBeenCalledWith(JSON.stringify(opArgs));
                });
            }

            var opArgs = {};
            for (i = 0; i < TestEnd2EndDatatypesTestData.length; ++i) {
                var test = TestEnd2EndDatatypesTestData[i];
                /* replace all dots with _ */
                var paramName = test.joynrType.replace(/\./g, "_") + "Arg";
                paramName = paramName.slice(0, 1).toLowerCase() + paramName.slice(1);
                opArgs[paramName] = test.values[0];
            }
            testMultipleArguments(opArgs);
        });

        afterEach(function() {
            var shutDownWW, shutDownLibJoynr;

            runs(function() {
                IntegrationUtils.shutdownWebWorker(workerId).then(function() {
                    shutDownWW = true;
                });
                IntegrationUtils.shutdownLibjoynr().then(function() {
                    shutDownLibJoynr = true;
                });
            });

            waitsFor(function() {
                return shutDownWW && shutDownLibJoynr;
            }, "WebWorker and Libjoynr to be shut down", 5000);
        });

    });
});