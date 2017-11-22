/*global fail: true, xit: true */
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

define( "integration/End2EndAbstractTest",
[
    "global/Promise",
    "joynr",
    "joynr/vehicle/RadioProxy",
    "integration/IntegrationUtils",
    "joynr/provisioning/provisioning_cc",
    "uuid",
    "global/WaitsFor"
],
function(
Promise,
joynr,
RadioProxy,
IntegrationUtils,
provisioning,
uuid,
waitsFor) {

    function End2EndAbstractTest(provisioningSuffix) {
        var radioProxy;
        var workerId;
        var testIdentifier = 0;

        jasmine.getEnv().defaultTimeoutInterval = 15000; //15 secs default timeout for async tests;
        this.beforeEach = function() {
            var provisioningSuffixForTest = provisioningSuffix + "-" + testIdentifier++;
            var domain = provisioningSuffixForTest;
            var testProvisioning = IntegrationUtils.getProvisioning(
            provisioning,
            domain);
            return joynr.load(testProvisioning).then(function(newjoynr) {
                joynr = newjoynr;
                IntegrationUtils.initialize(joynr);
                return new Promise(function(resolve, reject) {
                    IntegrationUtils.initializeWebWorker(
                    "TestEnd2EndCommProviderWorker",
                    provisioningSuffixForTest,
                    domain).then(function(newWorkerId) {
                        workerId = newWorkerId;
                        // Prevent freezing of object through proxy build
                        // since we need to add faked attribute below
                        spyOn(Object, 'freeze').and.callFake(function(obj) {
                            return obj;
                        });
                        return IntegrationUtils.buildProxy(RadioProxy, domain);
                    }).then(function(newRadioProxy) {
                        radioProxy = newRadioProxy;

                        // Add an attribute that does not exist on provider side
                        // for special subscription test
                        radioProxy.nonExistingAttributeOnProviderSide =
                        new radioProxy.settings.proxyElementTypes.ProxyAttributeNotifyReadWrite(radioProxy, radioProxy.settings, "nonExistingAttributeOnProviderSide", "Integer");

                        // restore freeze behavior
                        Object.freeze.and.callThrough();
                        radioProxy = Object.freeze(radioProxy);
                    }).then(function() {
                        return IntegrationUtils.startWebWorker(workerId);
                    }).then(function() {
                        resolve({
                            joynr: joynr,
                            radioProxy : radioProxy
                        });
                        return null;
                    });
                });
            });
        };

        /**
         * Calls attribute set with a certain value and tests whether attribute get
         * returns this value
         *
         * @param {ProxyAttribute}
         *            attribute the proxy attribute to test
         * @param {Number}
         *            index the recursion index that goes down to zero
         * @param {Function}
         *            [resolve] function to be invoked once the set async function has been completed
         * @param {Function}
         *            [reject] function to be invoked once the set async function fails
         */
        function setAndTestAttributeRecursive (attribute, index, resolve, reject) {

            // value is toggled between true/false on every increment of variable i,
            // ending at false on i=0
            var valueToBeSet = !!(index % 2);

            // set the value
            attribute.set({
                value : valueToBeSet
            }).then(
            function() {
                // get the value
                attribute.get().then(
                function(valueFromProvider) {
                    // check if provider set the value and returned
                    // the new one accordingly
                    IntegrationUtils.checkValueAndType(
                    valueFromProvider,
                    valueToBeSet);

                    // go on with the recursion
                    if (index === 0) {
                        // recursion should stop, index is decreased
                        // to 0
                        resolve(index);
                    } else {
                        // start next recursion level
                        setAndTestAttributeRecursive(
                        attribute,
                        index - 1,
                        resolve,
                        reject);
                    }
                    return null;
                }).catch(function(error) {
                    reject(new Error(
                    "Failed to retrieve attribute value (recursion index "
                    + index
                    + "): "
                    + error));
                });
                return null;
            }).catch(function(error) {
                reject(new Error(
                "Failed to set attribute value (recursion index "
                + index
                + "): "
                + error));
            });
            return null;
        }

        /**
         * Calls attribute set with a certain value and tests whether attribute get
         * returns this value
         *
         * @param {ProxyAttribute}
         *            attribute the proxy attribute to test
         * @param {Number}
         *            index the recursion index that goes down to zero
         * @returns {Promise} a promise object that rejects on the first occuring
         *          error or resolves with the value 0 if all tests finished
         *          successfully
         */
        this.setAndTestAttribute = function (attributeName, index) {
            return new Promise(function(resolve, reject) {
                setAndTestAttributeRecursive(radioProxy[attributeName], index, resolve, reject);
            });
        };

        this.getAttribute = function(attributeName, expectedValue) {
            var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");

            return waitsFor(function() {
                return radioProxy !== undefined;
            }, "radioProxy is defined", provisioning.ttl).then(function() {
                radioProxy[attributeName].get().then(function(value) {
                    expect(value).toEqual(expectedValue);
                    onFulfilledSpy(value);
                }).catch(function(error) {
                    return IntegrationUtils.outputPromiseError(new Error("End2EndAbstractTest.getAttribute. Error while getting: " + error.message));
                });

                return waitsFor(function() {
                    return onFulfilledSpy.calls.count() > 0;
                }, "attribute " + attributeName + " is received", provisioning.ttl);
            }).then(function() {
                expect(onFulfilledSpy).toHaveBeenCalled();
            });
        };

        this.getFailingAttribute = function(attributeName) {
            var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");
            var catchSpy = jasmine.createSpy("catchSpy");

            return waitsFor(function() {
                return radioProxy !== undefined;
            }, "radioProxy is defined", provisioning.ttl).then(function() {
                radioProxy[attributeName].get().then(function(value) {
                    onFulfilledSpy(value);
                }).catch(function(exception) {
                    catchSpy(exception);
                });

                return waitsFor(function() {
                    return catchSpy.calls.count() > 0;
                }, "getter for attribute " + attributeName + " returns exception", provisioning.ttl);
            }).then(function() {
                expect(catchSpy).toHaveBeenCalled();
                expect(catchSpy.calls.argsFor(0)[0]._typeName).toBeDefined();
                expect(catchSpy.calls.argsFor(0)[0]._typeName).toEqual("joynr.exceptions.ProviderRuntimeException");
            });
        };

        this.setAttribute = function(attributeName, value) {
            var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");

            radioProxy[attributeName].set({
                value : value
            }).then(onFulfilledSpy).catch(function(error) {
                return IntegrationUtils.outputPromiseError(new Error("End2EndAbstractTest.setAttribute. Error while setting: " + error.message));
            });

            return waitsFor(function() {
                return onFulfilledSpy.calls.count() > 0;
            }, "attribute is set", provisioning.ttl).then(function() {
                expect(onFulfilledSpy).toHaveBeenCalled();
            });
        };


        this.callOperation = function(
        operationName,
        opArgs,
        expectation) {
            var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");

            radioProxy[operationName](opArgs).then(
            onFulfilledSpy).catch(function(error) {
                return IntegrationUtils.outputPromiseError(new Error("End2EndAbstractTest.callOperation. Error while calling operation: " + error.message));
            });

            return waitsFor(function() {
                return onFulfilledSpy.calls.count() > 0;
            }, "operation call to finish", provisioning.ttl).then(function() {
                expect(onFulfilledSpy).toHaveBeenCalled();
                if (expectation !== undefined) {
                    if (typeof expectation === "function") {
                        expectation(onFulfilledSpy);
                    }
                    else {
                        expect(onFulfilledSpy).toHaveBeenCalledWith(expectation);
                    }
                }
            });
        };

        this.unsubscribeSubscription = function(subscribingEntity, subscriptionId) {
            return radioProxy[subscribingEntity].unsubscribe({
                subscriptionId : subscriptionId
            });
        };

        this.setupSubscriptionAndReturnSpy = function(subscribingEntity, subscriptionQos, partitions){
            var promise, spy = jasmine.createSpyObj("spy", [
                "onFulfilled",
                "onReceive",
                "onError"
            ]);

            promise = radioProxy[subscribingEntity].subscribe({
                subscriptionQos : subscriptionQos,
                partitions: partitions,
                onReceive : spy.onReceive,
                onError : spy.onError
            }).then(spy.onFulfilled).catch(function(error) {
                return IntegrationUtils.outputPromiseError(new Error("End2EndAbstractTest.setupSubscriptionAndReturnSpy. Error while subscribing: " + error.message));
            });

            return waitsFor(function() {
                return spy.onFulfilled.calls.count() > 0;
            }, "subscription to be registered", provisioning.ttl).then(function() {
                return Promise.resolve(spy);
            });
        };

        this.expectPublication = function(spy, expectationFct){
            return waitsFor(
            function() {
                return (spy.onReceive.calls.count() > 0 || spy.onError.calls.count() > 0);
            },
            "first publication to occur",
            500 + provisioning.ttl).then(function() {
                expect(spy.onReceive).toHaveBeenCalled();
                expect(spy.onError).not.toHaveBeenCalled();
                if (expectationFct) {
                    expectationFct(spy.onReceive.calls.mostRecent());
                }
                spy.onReceive.calls.reset();
            });
        };

        this.afterEach = function(done) {
            IntegrationUtils.shutdownWebWorker(workerId).then(function() {
                return IntegrationUtils.shutdownLibjoynr();
            }).then(function() {
                done();
                return null;
            }).catch(function() {
                throw new Error("shutdown Webworker and Libjoynr failed");
            });
        };
    }
    return End2EndAbstractTest;
});
