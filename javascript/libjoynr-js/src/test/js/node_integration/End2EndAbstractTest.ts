/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
import { ProxyReadWriteNotifyAttribute } from "../../../main/js/joynr/proxy/ProxyAttribute";
import RadioProxy from "../../generated/joynr/vehicle/RadioProxy";
import DatatypesProxy from "../../generated/joynr/datatypes/DatatypesProxy";
import MultipleVersionsInterfaceProxy from "../../generated/joynr/tests/MultipleVersionsInterfaceProxy";
import * as IntegrationUtils from "./IntegrationUtils";
import provisioning from "../../resources/joynr/provisioning/provisioning_cc";
import joynr from "../../../main/js/joynr";
import testUtil from "../../js/testUtil";
import SubscriptionQos = require("../../../main/js/joynr/proxy/SubscriptionQos");
import InProcessRuntime from "../../../main/js/joynr/start/InProcessRuntime";
import Mock = jest.Mock;

class End2EndAbstractTest {
    private processSpecialization: any;
    private providerChildProcessName: any;
    private provisioningSuffix: any;
    private testIdentifier: number = 0;
    private childId: any;
    private multipleVersionsInterfaceProxy!: MultipleVersionsInterfaceProxy;
    private dataProxy!: DatatypesProxy;
    private radioProxy!: RadioProxy;
    public constructor(provisioningSuffix: any, providerChildProcessName: any, processSpecialization?: any) {
        this.provisioningSuffix = provisioningSuffix;
        this.providerChildProcessName = providerChildProcessName;
        this.processSpecialization = processSpecialization;

        this.beforeEach = this.beforeEach.bind(this);
        this.afterEach = this.afterEach.bind(this);
        this.expectPublication = this.expectPublication.bind(this);
        this.getAttribute = this.getAttribute.bind(this);
        this.getFailingAttribute = this.getFailingAttribute.bind(this);
        this.setAndTestAttribute = this.setAndTestAttribute.bind(this);

        this.setAttribute = this.setAttribute.bind(this);
        this.setupSubscriptionAndReturnSpy = this.setupSubscriptionAndReturnSpy.bind(this);
        this.unsubscribeSubscription = this.unsubscribeSubscription.bind(this);
        this.callOperation = this.callOperation.bind(this);
        this.terminateAllSubscriptions = this.terminateAllSubscriptions.bind(this);
    }

    /**
     * Calls attribute set with a certain value and tests whether attribute get
     * returns this value
     *
     * @param attribute the proxy attribute to test
     * @param index the recursion index that goes down to zero
     * @param [resolve] function to be invoked once the set async function has been completed
     * @param [reject] function to be invoked once the set async function fails
     */
    private setAndTestAttributeRecursive(
        attribute: ProxyReadWriteNotifyAttribute<boolean>,
        index: number,
        resolve: Function,
        reject: Function
    ): void {
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
                            this.setAndTestAttributeRecursive(attribute, index - 1, resolve, reject);
                        }
                        return null;
                    })
                    .catch((error: any) => {
                        reject(new Error(`Failed to retrieve attribute value (recursion index ${index}): ${error}`));
                    });
                return null;
            })
            .catch((error: any) => {
                reject(new Error(`Failed to set attribute value (recursion index ${index}): ${error}`));
            });
    }

    public async beforeEach(): Promise<{
        joynr: joynr;
        radioProxy?: RadioProxy;
        dataProxy?: DatatypesProxy;
        multipleVersionsInterfaceProxy: MultipleVersionsInterfaceProxy;
    }> {
        // jest.resetModules();
        const provisioningSuffixForTest = `${this.provisioningSuffix}-${this.testIdentifier++}`;
        const domain = provisioningSuffixForTest;
        const testProvisioning: any = provisioning;
        testProvisioning.channelId = `abstract-test-base${provisioningSuffixForTest}`;
        // @ts-ignore
        joynr.loaded = false;
        process.removeAllListeners("exit");
        await joynr.selectRuntime(InProcessRuntime).load(testProvisioning);
        IntegrationUtils.initialize();
        this.childId = await IntegrationUtils.initializeChildProcess(
            this.providerChildProcessName,
            provisioningSuffixForTest,
            domain,
            this.processSpecialization
        );

        switch (this.providerChildProcessName) {
            case "TestEnd2EndDatatypesProviderProcess":
                this.dataProxy = await IntegrationUtils.buildProxy(DatatypesProxy, domain);
                break;
            case "TestEnd2EndCommProviderProcess":
                this.radioProxy = await IntegrationUtils.buildProxy(RadioProxy, domain);
                // Add an attribute that does not exist on provider side
                // for special subscription test
                // @ts-ignore
                this.radioProxy.nonExistingAttributeOnProviderSide = new ProxyReadWriteNotifyAttribute<number>(
                    this.radioProxy,
                    "nonExistingAttributeOnProviderSide",
                    "Integer"
                );
                break;
            case "TestMultipleVersionsInterfaceProcess": {
                const discoveryQos = new joynr.proxy.DiscoveryQos({
                    discoveryTimeoutMs: 1000
                });
                this.multipleVersionsInterfaceProxy = await joynr.proxyBuilder.build(MultipleVersionsInterfaceProxy, {
                    domain,
                    discoveryQos
                });
                break;
            }
            default:
                throw new Error("Please specify the process to invoke!");
        }

        await IntegrationUtils.startChildProcess(this.childId);
        return {
            joynr,
            radioProxy: this.radioProxy,
            dataProxy: this.dataProxy,
            multipleVersionsInterfaceProxy: this.multipleVersionsInterfaceProxy
        };
    }

    /**
     * Calls attribute set with a certain value and tests whether attribute get
     * returns this value
     *
     * @param attributeName of the proxy attribute to test
     * @param index the recursion index that goes down to zero
     * @returns a promise object that rejects on the first occuring
     *          error or resolves with the value 0 if all tests finished
     *          successfully
     */
    public setAndTestAttribute(attributeName: keyof RadioProxy, index: number): Promise<any> {
        return new Promise((resolve: any, reject: any) => {
            this.setAndTestAttributeRecursive(this.radioProxy[attributeName] as any, index, resolve, reject);
        });
    }

    public async getAttribute(attributeName: keyof RadioProxy, expectedValue: any) {
        const value = await ((this.radioProxy[attributeName] as any) as ProxyReadWriteNotifyAttribute<any>).get();
        expect(value).toEqual(expectedValue);
    }

    public async getFailingAttribute(attributeName: keyof RadioProxy) {
        const exception = await testUtil.reversePromise(
            ((this.radioProxy[attributeName] as any) as ProxyReadWriteNotifyAttribute<any>).get()
        );
        expect(exception._typeName).toBeDefined();
        expect(exception._typeName).toEqual("joynr.exceptions.ProviderRuntimeException");
    }

    public async setAttribute(attributeName: keyof RadioProxy, value: any) {
        await ((this.radioProxy[attributeName] as any) as ProxyReadWriteNotifyAttribute<any>).set({ value });
    }

    public async callOperation(operationName: keyof RadioProxy, opArgs: any, expectation?: any) {
        const result = await (this.radioProxy[operationName] as any)(opArgs);
        if (expectation !== undefined) {
            expect(result).toEqual(expectation);
        }
    }

    public unsubscribeSubscription(subscribingEntity: keyof RadioProxy, subscriptionId: string) {
        return ((this.radioProxy[subscribingEntity] as any) as ProxyReadWriteNotifyAttribute<any>).unsubscribe({
            subscriptionId
        });
    }

    public waitFor(condition: () => boolean, timeout: number): Promise<any> {
        const checkIntervalMs = 100;
        const start = Date.now();
        const check = (resolve: any, reject: any) => {
            if (condition()) {
                resolve();
            } else if (Date.now() > start + timeout) {
                reject(`${condition.name} failed, even after getting ${timeout} ms to try`);
            } else {
                setTimeout(_ => check(resolve, reject), checkIntervalMs);
            }
        };
        return new Promise(check);
    }

    public setupSubscriptionAndReturnSpy(
        subscribingEntity: keyof RadioProxy,
        subscriptionQos: SubscriptionQos,
        partitions?: string[]
    ) {
        const spy = {
            onFulfilled: jest.fn(),
            onReceive: jest.fn(),
            onError: jest.fn()
        };

        return ((this.radioProxy[subscribingEntity] as any) as ProxyReadWriteNotifyAttribute<any>)
            .subscribe({
                subscriptionQos,
                partitions,
                onReceive: spy.onReceive,
                onError: spy.onError
            } as any)
            .then(spy.onFulfilled)
            .then(() => spy);
    }

    public async expectPublication(spy: Record<string, Mock>, expectationFct?: Function) {
        const deferred = IntegrationUtils.createPromise();
        spy.onReceive.mockImplementation(deferred.resolve);
        spy.onError.mockImplementation(deferred.reject);
        if (spy.onReceive.mock.calls.length > 0) deferred.resolve();
        if (spy.onError.mock.calls.length > 0) deferred.reject();

        await deferred.promise;
        expect(spy.onReceive).toHaveBeenCalled();
        expect(spy.onError).not.toHaveBeenCalled();
        if (expectationFct) {
            expectationFct(spy.onReceive.mock.calls.slice(-1)[0]);
        }
        spy.onReceive.mockClear();
    }

    public async afterEach() {
        return IntegrationUtils.shutdownChildProcess(this.childId)
            .then(() => {
                return IntegrationUtils.shutdownLibjoynr();
            })
            .catch((e: any) => {
                throw new Error(`shutdown Child and Libjoynr failed: ${e}`);
            });
    }

    public async terminateAllSubscriptions() {
        // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
        return joynr.terminateAllSubscriptions!();
    }
}

export = End2EndAbstractTest;
