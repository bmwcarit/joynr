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
let RadioProxy = require("../../generated/joynr/vehicle/RadioProxy"),
    DatatypesProxy = require("../../generated/joynr/datatypes/DatatypesProxy"),
    IntegrationUtils = require("./IntegrationUtils"),
    provisioning = require("../../resources/joynr/provisioning/provisioning_cc"),
    waitsFor = require("../global/WaitsFor");
const joynr = require("joynr");

function End2EndAbstractTest(provisioningSuffix, buildDataProxy) {
    let radioProxy, dataProxy, loadedJoynr;
    let childId;
    let testIdentifier = 0;

    this.beforeEach = function() {
        const provisioningSuffixForTest = provisioningSuffix + "-" + testIdentifier++;
        const domain = provisioningSuffixForTest;
        provisioning.channelId = "abstract-test-base" + provisioningSuffixForTest;
        const testProvisioning = provisioning;
        joynr.loaded = false;
        joynr.selectRuntime("inprocess");
        return joynr
            .load(testProvisioning)
            .then(newJoynr => {
                loadedJoynr = newJoynr;
                IntegrationUtils.initialize(loadedJoynr);
                const providerChildProcessName = buildDataProxy
                    ? "TestEnd2EndDatatypesProviderProcess"
                    : "TestEnd2EndCommProviderProcess";
                return IntegrationUtils.initializeChildProcess(
                    providerChildProcessName,
                    provisioningSuffixForTest,
                    domain
                );
            })
            .then(newChildId => {
                childId = newChildId;
                if (buildDataProxy) {
                    return IntegrationUtils.buildProxy(DatatypesProxy, domain).then(newDataProxy => {
                        dataProxy = newDataProxy;
                    });
                }

                // Prevent freezing of object through proxy build
                // since we need to add faked attribute below
                spyOn(Object, "freeze").and.callFake(obj => {
                    return obj;
                });
                return IntegrationUtils.buildProxy(RadioProxy, domain).then(newRadioProxy => {
                    radioProxy = newRadioProxy;

                    // Add an attribute that does not exist on provider side
                    // for special subscription test
                    radioProxy.nonExistingAttributeOnProviderSide = new radioProxy.settings.proxyElementTypes.ProxyAttribute(
                        radioProxy,
                        radioProxy.settings,
                        "nonExistingAttributeOnProviderSide",
                        "Integer",
                        "NOTIFYREADWRITE"
                    );

                    // restore freeze behavior
                    Object.freeze.and.callThrough();
                    radioProxy = Object.freeze(radioProxy);
                });
            })
            .then(() => {
                return IntegrationUtils.startChildProcess(childId);
            })
            .then(() => {
                return Promise.resolve({
                    joynr: loadedJoynr,
                    radioProxy,
                    dataProxy
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
    function setAndTestAttributeRecursive(attribute, index, resolve, reject) {
        // value is toggled between true/false on every increment of variable i,
        // ending at false on i=0
        const valueToBeSet = !!(index % 2);

        // set the value
        attribute
            .set({
                value: valueToBeSet
            })
            .then(() => {
                // get the value
                attribute
                    .get()
                    .then(valueFromProvider => {
                        // check if provider set the value and returned
                        // the new one accordingly
                        IntegrationUtils.checkValueAndType(valueFromProvider, valueToBeSet);

                        // go on with the recursion
                        if (index === 0) {
                            // recursion should stop, index is decreased
                            // to 0
                            resolve(index);
                        } else {
                            // start next recursion level
                            setAndTestAttributeRecursive(attribute, index - 1, resolve, reject);
                        }
                        return null;
                    })
                    .catch(error => {
                        reject(
                            new Error("Failed to retrieve attribute value (recursion index " + index + "): " + error)
                        );
                    });
                return null;
            })
            .catch(error => {
                reject(new Error("Failed to set attribute value (recursion index " + index + "): " + error));
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
    this.setAndTestAttribute = function(attributeName, index) {
        return new Promise((resolve, reject) => {
            setAndTestAttributeRecursive(radioProxy[attributeName], index, resolve, reject);
        });
    };

    this.getAttribute = function(attributeName, expectedValue) {
        const onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");

        return waitsFor(
            () => {
                return radioProxy !== undefined;
            },
            "radioProxy is defined",
            provisioning.ttl
        )
            .then(() => {
                radioProxy[attributeName]
                    .get()
                    .then(value => {
                        expect(value).toEqual(expectedValue);
                        onFulfilledSpy(value);
                    })
                    .catch(error => {
                        return IntegrationUtils.outputPromiseError(
                            new Error("End2EndAbstractTest.getAttribute. Error while getting: " + error.message)
                        );
                    });

                return waitsFor(
                    () => {
                        return onFulfilledSpy.calls.count() > 0;
                    },
                    "attribute " + attributeName + " is received",
                    provisioning.ttl
                );
            })
            .then(() => {
                expect(onFulfilledSpy).toHaveBeenCalled();
            });
    };

    this.getFailingAttribute = function(attributeName) {
        const onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");
        const catchSpy = jasmine.createSpy("catchSpy");

        return waitsFor(
            () => {
                return radioProxy !== undefined;
            },
            "radioProxy is defined",
            provisioning.ttl
        )
            .then(() => {
                radioProxy[attributeName]
                    .get()
                    .then(value => {
                        onFulfilledSpy(value);
                    })
                    .catch(exception => {
                        catchSpy(exception);
                    });

                return waitsFor(
                    () => {
                        return catchSpy.calls.count() > 0;
                    },
                    "getter for attribute " + attributeName + " returns exception",
                    provisioning.ttl
                );
            })
            .then(() => {
                expect(catchSpy).toHaveBeenCalled();
                expect(catchSpy.calls.argsFor(0)[0]._typeName).toBeDefined();
                expect(catchSpy.calls.argsFor(0)[0]._typeName).toEqual("joynr.exceptions.ProviderRuntimeException");
            });
    };

    this.setAttribute = function(attributeName, value) {
        const onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");

        radioProxy[attributeName]
            .set({
                value
            })
            .then(onFulfilledSpy)
            .catch(error => {
                return IntegrationUtils.outputPromiseError(
                    new Error("End2EndAbstractTest.setAttribute. Error while setting: " + error.message)
                );
            });

        return waitsFor(
            () => {
                return onFulfilledSpy.calls.count() > 0;
            },
            "attribute is set",
            provisioning.ttl
        ).then(() => {
            expect(onFulfilledSpy).toHaveBeenCalled();
        });
    };

    this.callOperation = function(operationName, opArgs, expectation) {
        const onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");

        radioProxy[operationName](opArgs)
            .then(onFulfilledSpy)
            .catch(error => {
                return IntegrationUtils.outputPromiseError(
                    new Error("End2EndAbstractTest.callOperation. Error while calling operation: " + error.message)
                );
            });

        return waitsFor(
            () => {
                return onFulfilledSpy.calls.count() > 0;
            },
            "operation call to finish",
            provisioning.ttl
        ).then(() => {
            expect(onFulfilledSpy).toHaveBeenCalled();
            if (expectation !== undefined) {
                if (typeof expectation === "function") {
                    expectation(onFulfilledSpy);
                } else {
                    expect(onFulfilledSpy).toHaveBeenCalledWith(expectation);
                }
            }
        });
    };

    this.unsubscribeSubscription = function(subscribingEntity, subscriptionId) {
        return radioProxy[subscribingEntity].unsubscribe({
            subscriptionId
        });
    };

    this.setupSubscriptionAndReturnSpy = function(subscribingEntity, subscriptionQos, partitions) {
        let spy = jasmine.createSpyObj("spy", ["onFulfilled", "onReceive", "onError"]);

        radioProxy[subscribingEntity]
            .subscribe({
                subscriptionQos,
                partitions,
                onReceive: spy.onReceive,
                onError: spy.onError
            })
            .then(spy.onFulfilled)
            .catch(error => {
                return IntegrationUtils.outputPromiseError(
                    new Error(
                        "End2EndAbstractTest.setupSubscriptionAndReturnSpy. Error while subscribing: " + error.message
                    )
                );
            });

        return waitsFor(
            () => {
                return spy.onFulfilled.calls.count() > 0;
            },
            "subscription to be registered",
            provisioning.ttl
        ).then(() => {
            return Promise.resolve(spy);
        });
    };

    this.expectPublication = function(spy, expectationFct) {
        return waitsFor(
            () => {
                return spy.onReceive.calls.count() > 0 || spy.onError.calls.count() > 0;
            },
            "first publication to occur",
            500 + provisioning.ttl
        ).then(() => {
            expect(spy.onReceive).toHaveBeenCalled();
            expect(spy.onError).not.toHaveBeenCalled();
            if (expectationFct) {
                expectationFct(spy.onReceive.calls.mostRecent());
            }
            spy.onReceive.calls.reset();
        });
    };

    this.afterEach = function(done) {
        IntegrationUtils.shutdownChildProcess(childId)
            .then(() => {
                return IntegrationUtils.shutdownLibjoynr();
            })
            .then(() => {
                done();
                return null;
            })
            .catch(e => {
                throw new Error("shutdown Child and Libjoynr failed: " + e);
            });
    };
}
module.exports = End2EndAbstractTest;
