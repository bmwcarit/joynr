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

const RadioProxy = require("../../generated/joynr/vehicle/RadioProxy");
const DatatypesProxy = require("../../generated/joynr/datatypes/DatatypesProxy");
const MultipleVersionsInterfaceProxy = require("../../generated/joynr/tests/MultipleVersionsInterfaceProxy");
const IntegrationUtils = require("./IntegrationUtils");
const provisioning = require("../../resources/joynr/provisioning/provisioning_cc");
let joynr = require("joynr");
const testUtil = require("../../js/testUtil");

function End2EndAbstractTest(provisioningSuffix, providerChildProcessName, processSpecialization) {
    let radioProxy, dataProxy, multipleVersionsInterfaceProxy;
    let childId;
    let testIdentifier = 0;

    this.beforeEach = async function() {
        const provisioningSuffixForTest = `${provisioningSuffix}-${testIdentifier++}`;
        const domain = provisioningSuffixForTest;
        provisioning.channelId = `abstract-test-base${provisioningSuffixForTest}`;
        const testProvisioning = provisioning;
        joynr.loaded = false;
        joynr.selectRuntime("inprocess");

        await joynr.load(testProvisioning);
        IntegrationUtils.initialize(joynr);
        childId = await IntegrationUtils.initializeChildProcess(
            providerChildProcessName,
            provisioningSuffixForTest,
            domain,
            processSpecialization
        );

        switch (providerChildProcessName) {
            case "TestEnd2EndDatatypesProviderProcess":
                dataProxy = await IntegrationUtils.buildProxy(DatatypesProxy, domain);
                break;
            case "TestEnd2EndCommProviderProcess":
                // Prevent freezing of object through proxy build
                // since we need to add faked attribute below
                spyOn(Object, "freeze").and.callFake(obj => {
                    return obj;
                });
                radioProxy = await IntegrationUtils.buildProxy(RadioProxy, domain);
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
                break;
            case "TestMultipleVersionsInterfaceProcess": {
                const discoveryQos = new joynr.proxy.DiscoveryQos({
                    discoveryTimeoutMs: 1000
                });
                multipleVersionsInterfaceProxy = await joynr.proxyBuilder.build(MultipleVersionsInterfaceProxy, {
                    domain,
                    discoveryQos
                });
                break;
            }
            default:
                throw new Error("Please specify the process to invoke!");
        }

        await IntegrationUtils.startChildProcess(childId);
        return {
            joynr,
            radioProxy,
            dataProxy,
            multipleVersionsInterfaceProxy
        };
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
                        reject(new Error(`Failed to retrieve attribute value (recursion index ${index}): ${error}`));
                    });
                return null;
            })
            .catch(error => {
                reject(new Error(`Failed to set attribute value (recursion index ${index}): ${error}`));
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

    this.getAttribute = async function(attributeName, expectedValue) {
        const value = await radioProxy[attributeName].get();
        expect(value).toEqual(expectedValue);
    };

    this.getFailingAttribute = async function(attributeName) {
        const exception = await testUtil.reversePromise(radioProxy[attributeName].get());
        expect(exception._typeName).toBeDefined();
        expect(exception._typeName).toEqual("joynr.exceptions.ProviderRuntimeException");
    };

    this.setAttribute = async function(attributeName, value) {
        await radioProxy[attributeName].set({ value });
    };

    this.callOperation = async function(operationName, opArgs, expectation) {
        const result = await radioProxy[operationName](opArgs);
        if (expectation !== undefined) {
            expect(result).toEqual(expectation);
        }
    };

    this.unsubscribeSubscription = function(subscribingEntity, subscriptionId) {
        return radioProxy[subscribingEntity].unsubscribe({
            subscriptionId
        });
    };

    this.setupSubscriptionAndReturnSpy = function(subscribingEntity, subscriptionQos, partitions) {
        const spy = jasmine.createSpyObj("spy", ["onFulfilled", "onReceive", "onError"]);

        return radioProxy[subscribingEntity]
            .subscribe({
                subscriptionQos,
                partitions,
                onReceive: spy.onReceive,
                onError: spy.onError
            })
            .then(spy.onFulfilled)
            .then(() => spy);
    };

    this.expectPublication = function(spy, expectationFct) {
        const deferred = IntegrationUtils.createPromise();
        spy.onReceive.and.callFake(deferred.resolve);
        spy.onError.and.callFake(deferred.reject);
        if (spy.onReceive.calls.any()) deferred.resolve();
        if (spy.onError.calls.any()) deferred.reject();

        return deferred.promise.then(() => {
            expect(spy.onReceive).toHaveBeenCalled();
            expect(spy.onError).not.toHaveBeenCalled();
            if (expectationFct) {
                expectationFct(spy.onReceive.calls.mostRecent());
            }
            spy.onReceive.calls.reset();
        });
    };

    this.afterEach = async function() {
        return IntegrationUtils.shutdownChildProcess(childId)
            .then(() => {
                return IntegrationUtils.shutdownLibjoynr();
            })
            .then(() => {
                delete require.cache;
                // remove old joynr exit handler
                process.removeAllListeners("exit");
                /*eslint-disable */
                joynr = require("joynr");
                /*eslint-enable */
            })
            .catch(e => {
                throw new Error(`shutdown Child and Libjoynr failed: ${e}`);
            });
    };

    this.terminateAllSubscriptions = async function() {
        return joynr.terminateAllSubscriptions();
    };
}
module.exports = End2EndAbstractTest;
