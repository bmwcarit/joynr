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

import * as DiscoveryError from "../../../../../main/js/generated/joynr/types/DiscoveryError";
import * as ProviderScope from "../../../../../main/js/generated/joynr/types/ProviderScope";
import Arbitrator from "../../../../../main/js/joynr/capabilities/arbitration/Arbitrator";
import DiscoveryEntryWithMetaInfo from "../../../../../main/js/generated/joynr/types/DiscoveryEntryWithMetaInfo";
import ProviderQos from "../../../../../main/js/generated/joynr/types/ProviderQos";
import CustomParameter from "../../../../../main/js/generated/joynr/types/CustomParameter";
import DiscoveryQos from "../../../../../main/js/joynr/proxy/DiscoveryQos";
import * as ArbitrationStrategyCollection from "../../../../../main/js/joynr/types/ArbitrationStrategyCollection";
import DiscoveryScope from "../../../../../main/js/generated/joynr/types/DiscoveryScope";
import DiscoveryException from "../../../../../main/js/joynr/exceptions/DiscoveryException";
import NoCompatibleProviderFoundException from "../../../../../main/js/joynr/exceptions/NoCompatibleProviderFoundException";
import Version from "../../../../../main/js/generated/joynr/types/Version";
import * as UtilInternal from "../../../../../main/js/joynr/util/UtilInternal";
import * as testUtil from "../../../testUtil";
import ApplicationException = require("../../../../../main/js/joynr/exceptions/ApplicationException");
import { FIXED_PARTICIPANT_PARAMETER } from "joynr/joynr/types/ArbitrationConstants";

let capabilities: any, fakeTime: number, staticArbitrationSettings: any, domain: any;
let interfaceName: string,
    discoveryQos: DiscoveryQos,
    capDiscoverySpy: any,
    arbitrator: Arbitrator,
    discoveryEntries: DiscoveryEntryWithMetaInfo[],
    nrTimes: any;
let fixedParticipantIdDiscoveryEntryWithMetaInfo: DiscoveryEntryWithMetaInfo;
let discoveryEntryWithMajor47AndMinor0: any, discoveryEntryWithMajor47AndMinor1: any;
let discoveryEntryWithMajor47AndMinor2: any, discoveryEntryWithMajor47AndMinor3: any;
let discoveryEntryWithMajor48AndMinor2: any;
// save values once before installing jasmine clock mocks

async function increaseFakeTime(timeMs: number): Promise<void> {
    fakeTime = fakeTime + timeMs;
    jest.advanceTimersByTime(timeMs);
    await testUtil.multipleSetImmediate();
}

function getDiscoveryEntry(
    domain: any,
    interfaceName: string,
    providerVersion: any,
    supportsOnChangeSubscriptions: any
) {
    return new DiscoveryEntryWithMetaInfo({
        providerVersion,
        domain,
        interfaceName,
        qos: new ProviderQos({
            customParameters: [new CustomParameter({ name: "theName", value: "theValue" })],
            priority: 123,
            scope:
                discoveryQos.discoveryScope === DiscoveryScope.LOCAL_ONLY ? ProviderScope.LOCAL : ProviderScope.GLOBAL,
            supportsOnChangeSubscriptions
        }),
        participantId: "700",
        lastSeenDateMs: Date.now(),
        publicKeyId: "",
        isLocal: true,
        expiryDateMs: Date.now() + 1e10
    });
}

describe("libjoynr-js.joynr.capabilities.arbitration.Arbitrator", () => {
    capabilities = [
        {
            domain: "myDomain",
            interfaceName: "myInterface",
            participantId: 1,
            providerVersion: new Version({ majorVersion: 47, minorVersion: 11 }),
            qos: {
                supportsOnChangeSubscriptions: true
            }
        },
        {
            domain: "myDomain",
            interfaceName: "myInterface",
            participantId: 2,
            providerVersion: new Version({ majorVersion: 47, minorVersion: 11 }),
            qos: {
                supportsOnChangeSubscriptions: false
            }
        },
        {
            domain: "otherDomain",
            interfaceName: "otherInterface",
            participantId: 3,
            providerVersion: new Version({ majorVersion: 47, minorVersion: 11 }),
            qos: {
                supportsOnChangeSubscriptions: true
            }
        },
        {
            domain: "thirdDomain",
            interfaceName: "otherInterface",
            participantId: 4,
            providerVersion: new Version({ majorVersion: 47, minorVersion: 11 }),
            qos: {
                supportsOnChangeSubscriptions: false
            }
        }
    ];

    beforeEach(() => {
        domain = "myDomain";
        interfaceName = "myInterface";
        discoveryQos = new DiscoveryQos({
            discoveryTimeoutMs: 5000,
            discoveryRetryDelayMs: 900,
            arbitrationStrategy: ArbitrationStrategyCollection.Nothing,
            cacheMaxAgeMs: 0,
            discoveryScope: DiscoveryScope.LOCAL_THEN_GLOBAL,
            additionalParameters: {}
        });

        staticArbitrationSettings = {
            domains: [domain],
            interfaceName,
            discoveryQos,
            staticArbitration: true,
            proxyVersion: new Version({ majorVersion: 47, minorVersion: 11 })
        };

        capDiscoverySpy = {
            lookup: jest.fn(),
            lookupByParticipantId: jest.fn()
        };
        capDiscoverySpy.lookup.mockReturnValue(Promise.resolve([]));

        arbitrator = new Arbitrator(capDiscoverySpy, capabilities);

        discoveryEntries = [];
        for (let i = 0; i < 12; ++i) {
            discoveryEntries.push(
                getDiscoveryEntry(
                    domain + i.toString(),
                    interfaceName + i.toString(),
                    new Version({ majorVersion: 47, minorVersion: 11 }),
                    false
                )
            );
        }

        // prepare a number of similar discovery entries with different
        // provider versions

        const providerQos = new ProviderQos({
            customParameters: [],
            priority: 123,
            scope:
                discoveryQos.discoveryScope === DiscoveryScope.LOCAL_ONLY ? ProviderScope.LOCAL : ProviderScope.GLOBAL,
            supportsOnChangeSubscriptions: true
        });

        discoveryEntryWithMajor47AndMinor0 = new DiscoveryEntryWithMetaInfo({
            providerVersion: new Version({ majorVersion: 47, minorVersion: 0 }),
            domain,
            interfaceName,
            qos: providerQos,
            participantId: "700",
            lastSeenDateMs: Date.now(),
            publicKeyId: "",
            isLocal: true,
            expiryDateMs: Date.now() + 1e10
        });

        discoveryEntryWithMajor47AndMinor1 = new DiscoveryEntryWithMetaInfo({
            providerVersion: new Version({ majorVersion: 47, minorVersion: 1 }),
            domain,
            interfaceName,
            qos: providerQos,
            participantId: "700",
            lastSeenDateMs: Date.now(),
            publicKeyId: "",
            isLocal: false,
            expiryDateMs: Date.now() + 1e10
        });

        discoveryEntryWithMajor47AndMinor2 = new DiscoveryEntryWithMetaInfo({
            providerVersion: new Version({ majorVersion: 47, minorVersion: 2 }),
            domain,
            interfaceName,
            qos: providerQos,
            participantId: "700",
            lastSeenDateMs: Date.now(),
            publicKeyId: "",
            isLocal: true,
            expiryDateMs: Date.now() + 1e10
        });

        discoveryEntryWithMajor47AndMinor3 = new DiscoveryEntryWithMetaInfo({
            providerVersion: new Version({ majorVersion: 47, minorVersion: 3 }),
            domain,
            interfaceName,
            qos: providerQos,
            participantId: "700",
            lastSeenDateMs: Date.now(),
            publicKeyId: "",
            isLocal: false,
            expiryDateMs: Date.now() + 1e10
        });

        discoveryEntryWithMajor48AndMinor2 = new DiscoveryEntryWithMetaInfo({
            providerVersion: new Version({ majorVersion: 48, minorVersion: 2 }),
            domain,
            interfaceName,
            qos: providerQos,
            participantId: "700",
            lastSeenDateMs: Date.now(),
            publicKeyId: "",
            isLocal: true,
            expiryDateMs: Date.now() + 1e10
        });

        nrTimes = 5;
        fakeTime = 0;

        jest.useFakeTimers();
        jest.spyOn(Date, "now").mockImplementation(() => {
            return fakeTime;
        });
    });

    afterEach(() => {
        jest.useRealTimers();
    });

    describe("ArbitrationStrategy.FixedParticipant", () => {
        const expectedFixedParticipantId = "expectedFixedParticipantId";

        beforeEach(() => {
            discoveryQos.arbitrationStrategy = ArbitrationStrategyCollection.FixedParticipant;
            discoveryQos.additionalParameters = { [FIXED_PARTICIPANT_PARAMETER]: expectedFixedParticipantId };

            const expiryDateMs = Date.now() + 1e10;
            fixedParticipantIdDiscoveryEntryWithMetaInfo = new DiscoveryEntryWithMetaInfo({
                providerVersion: new Version({
                    majorVersion: 47,
                    minorVersion: 11
                }),
                domain,
                interfaceName,
                lastSeenDateMs: 111,
                qos: new ProviderQos({
                    customParameters: [],
                    priority: 1,
                    scope: ProviderScope.GLOBAL,
                    supportsOnChangeSubscriptions: false
                }),
                participantId: expectedFixedParticipantId,
                isLocal: false,
                publicKeyId: "",
                expiryDateMs
            });

            capDiscoverySpy.lookupByParticipantId.mockReturnValue(
                Promise.resolve(fixedParticipantIdDiscoveryEntryWithMetaInfo)
            );
        });

        it("calls lookup by participantId and returns entry with expected participantId", async () => {
            // start arbitration
            const result = await arbitrator.startArbitration({
                domains: [domain],
                interfaceName,
                discoveryQos,
                proxyVersion: new Version({ majorVersion: 47, minorVersion: 11 })
            });

            expect(capDiscoverySpy.lookupByParticipantId).toHaveBeenCalled();
            expect(capDiscoverySpy.lookupByParticipantId.mock.calls.slice(-1)[0][0]).toEqual(
                expectedFixedParticipantId
            );
            expect(capDiscoverySpy.lookupByParticipantId.mock.calls.slice(-1)[0][1].cacheMaxAge).toEqual(
                discoveryQos.cacheMaxAgeMs
            );
            expect(capDiscoverySpy.lookupByParticipantId.mock.calls.slice(-1)[0][1].discoveryScope.name).toEqual(
                discoveryQos.discoveryScope.name
            );
            expect(capDiscoverySpy.lookupByParticipantId.mock.calls.slice(-1)[0][1].discoveryTimeout).toEqual(
                discoveryQos.discoveryTimeoutMs
            );
            expect(
                capDiscoverySpy.lookupByParticipantId.mock.calls.slice(-1)[0][1].providerMustSupportOnChange
            ).toEqual(discoveryQos.providerMustSupportOnChange);
            expect(capDiscoverySpy.lookupByParticipantId.mock.calls.slice(-1)[0][2]).toEqual([]);

            expect(result).toEqual([fixedParticipantIdDiscoveryEntryWithMetaInfo]);
        });

        it("calls capabilityDiscovery with provided gbid array", async () => {
            const gbids = ["joynrtestgbid"];

            // start arbitration
            await arbitrator.startArbitration({
                domains: [domain],
                interfaceName,
                discoveryQos,
                proxyVersion: new Version({ majorVersion: 47, minorVersion: 11 }),
                gbids
            });
            expect(capDiscoverySpy.lookupByParticipantId).toHaveBeenCalledWith(
                expectedFixedParticipantId,
                expect.any(Object),
                gbids
            );
        });
    });

    it("is instantiable", () => {
        expect(new Arbitrator({} as any)).toBeDefined();
    });

    it("is of correct type and has all members", () => {
        expect(arbitrator).toBeDefined();
        expect(arbitrator).not.toBeNull();
        expect(typeof arbitrator === "object").toBeTruthy();
        expect(arbitrator instanceof Arbitrator).toEqual(true);
        expect(arbitrator.startArbitration).toBeDefined();
        expect(typeof arbitrator.startArbitration === "function").toEqual(true);
    });

    it("calls capabilityDiscovery upon arbitration", async () => {
        // return some discoveryEntries so that arbitration is faster
        // (instantly instead of discoveryTimeout)
        capDiscoverySpy.lookup.mockReturnValue(Promise.resolve(discoveryEntries));
        jest.spyOn(discoveryQos, "arbitrationStrategy").mockReturnValue(discoveryEntries);
        arbitrator = new Arbitrator(capDiscoverySpy);

        // start arbitration
        await arbitrator.startArbitration({
            domains: [domain],
            interfaceName,
            discoveryQos,
            proxyVersion: new Version({ majorVersion: 47, minorVersion: 11 })
        });
        expect(capDiscoverySpy.lookup).toHaveBeenCalled();
        /* The arbitrator.startArbitration does a deep copy of its arguments.
         * Thus, two discoveryScope objects cannot be compared, as during deep copy
         * complex types are created as pure objects
         */
        expect(capDiscoverySpy.lookup.mock.calls.slice(-1)[0][0]).toEqual([domain]);
        expect(capDiscoverySpy.lookup.mock.calls.slice(-1)[0][1]).toEqual(interfaceName);
        expect(capDiscoverySpy.lookup.mock.calls.slice(-1)[0][2].cacheMaxAge).toEqual(discoveryQos.cacheMaxAgeMs);
        expect(capDiscoverySpy.lookup.mock.calls.slice(-1)[0][2].discoveryScope.name).toEqual(
            discoveryQos.discoveryScope.name
        );
    });

    it("calls capabilityDiscovery with provided gbid array", async () => {
        capDiscoverySpy.lookup.mockReturnValue(Promise.resolve(discoveryEntries));
        jest.spyOn(discoveryQos, "arbitrationStrategy").mockReturnValue(discoveryEntries);
        const gbids = ["joynrdefaultgbid"];

        // start arbitration
        await arbitrator.startArbitration({
            domains: [domain],
            interfaceName,
            discoveryQos,
            proxyVersion: new Version({ majorVersion: 47, minorVersion: 11 }),
            gbids
        });
        expect(capDiscoverySpy.lookup).toHaveBeenCalledWith([domain], interfaceName, expect.any(Object), gbids);
    });

    it("calls capabilityDiscovery with empty gbid array when unspecified", async () => {
        capDiscoverySpy.lookup.mockReturnValue(Promise.resolve(discoveryEntries));
        jest.spyOn(discoveryQos, "arbitrationStrategy").mockReturnValue(discoveryEntries);

        // start arbitration
        await arbitrator.startArbitration({
            domains: [domain],
            interfaceName,
            discoveryQos,
            proxyVersion: new Version({ majorVersion: 47, minorVersion: 11 })
        });
        expect(capDiscoverySpy.lookup).toHaveBeenCalledWith([domain], interfaceName, expect.any(Object), []);
    });

    async function returnCapabilitiesFromDiscovery(
        providerMustSupportOnChange: boolean,
        discoveryEntries: any,
        expected: any
    ) {
        // return discoveryEntries to check whether these are eventually
        // returned by the arbitrator
        capDiscoverySpy.lookup.mockReturnValue(Promise.resolve(discoveryEntries));
        arbitrator = new Arbitrator(capDiscoverySpy);

        const result = await arbitrator.startArbitration({
            domains: [domain],
            interfaceName,
            discoveryQos: new DiscoveryQos(UtilInternal.extend(discoveryQos, { providerMustSupportOnChange })),
            proxyVersion: new Version({ majorVersion: 47, minorVersion: 11 })
        });
        expect(result).toEqual(expected);
    }

    function setSupportsOnChangeSubscriptionsToTrue(discoveryEntry: any) {
        discoveryEntry.qos = new ProviderQos(
            UtilInternal.extend(discoveryEntry.qos, {
                supportsOnChangeSubscriptions: true
            })
        );
    }

    it("returns capabilities from discovery", () => {
        return returnCapabilitiesFromDiscovery(false, discoveryEntries, discoveryEntries);
    });

    it("returns filtered capabilities from discovery if discoveryQos.providerMustSupportOnChange is true", () => {
        setSupportsOnChangeSubscriptionsToTrue(discoveryEntries[1]);
        setSupportsOnChangeSubscriptionsToTrue(discoveryEntries[5]);
        setSupportsOnChangeSubscriptionsToTrue(discoveryEntries[11]);
        const filteredDiscoveryEntries = [discoveryEntries[1], discoveryEntries[5], discoveryEntries[11]];
        return returnCapabilitiesFromDiscovery(true, discoveryEntries, filteredDiscoveryEntries);
    });

    it("returns capabilities with matching provider version", async () => {
        const discoveryEntriesWithDifferentProviderVersions = [
            discoveryEntryWithMajor47AndMinor0,
            discoveryEntryWithMajor47AndMinor1,
            discoveryEntryWithMajor47AndMinor2,
            discoveryEntryWithMajor47AndMinor3,
            discoveryEntryWithMajor48AndMinor2
        ];

        // return discoveryEntries to check whether these are eventually
        // returned by the arbitrator
        capDiscoverySpy.lookup.mockResolvedValue(discoveryEntriesWithDifferentProviderVersions);
        arbitrator = new Arbitrator(capDiscoverySpy);

        const returnedDiscoveryEntries = await arbitrator.startArbitration({
            domains: [domain],
            interfaceName,
            discoveryQos,
            proxyVersion: new Version({ majorVersion: 47, minorVersion: 2 })
        });

        // remove lastSeenDateMs from object since it has been modified
        // by the lookup attempt
        // by providerVersion because of ArbitrationStrategyCollection.Nothing
        expect(returnedDiscoveryEntries).not.toContain(discoveryEntryWithMajor47AndMinor0);
        expect(returnedDiscoveryEntries).not.toContain(discoveryEntryWithMajor47AndMinor1);
        expect(returnedDiscoveryEntries).toContain(discoveryEntryWithMajor47AndMinor2);
        expect(returnedDiscoveryEntries).toContain(discoveryEntryWithMajor47AndMinor3);
        expect(returnedDiscoveryEntries).not.toContain(discoveryEntryWithMajor48AndMinor2);
    });

    it("rejects with NoCompatibleProviderFoundException including a list of incompatible provider version of latest lookup", async () => {
        const expectedMinimumMinorVersion = 2;
        const firstLookupResult = [
            discoveryEntryWithMajor47AndMinor0,
            discoveryEntryWithMajor47AndMinor1,
            discoveryEntryWithMajor47AndMinor2,
            discoveryEntryWithMajor47AndMinor3,
            discoveryEntryWithMajor48AndMinor2
        ];
        const secondLookupResult = [discoveryEntryWithMajor47AndMinor0, discoveryEntryWithMajor48AndMinor2];

        // return discoveryEntries to check whether these are eventually
        // returned by the arbitrator
        capDiscoverySpy.lookup.mockResolvedValue(firstLookupResult);
        arbitrator = new Arbitrator(capDiscoverySpy);

        const discoveryQosWithShortTimers = new DiscoveryQos({
            discoveryTimeoutMs: 1000,
            discoveryRetryDelayMs: 600,
            arbitrationStrategy: ArbitrationStrategyCollection.Nothing,
            cacheMaxAgeMs: 0,
            discoveryScope: DiscoveryScope.LOCAL_THEN_GLOBAL,
            additionalParameters: {}
        });

        // spy on and instrument arbitrationStrategy
        jest.spyOn(discoveryQosWithShortTimers, "arbitrationStrategy");

        // call arbitrator
        const onFulfilledSpy = jest.fn();
        const onRejectedSpy = jest.fn();

        arbitrator
            .startArbitration({
                domains: [domain],
                interfaceName,
                discoveryQos: discoveryQosWithShortTimers,
                proxyVersion: new Version({
                    majorVersion: 49,
                    minorVersion: expectedMinimumMinorVersion
                })
            })
            .then(onFulfilledSpy)
            .catch(onRejectedSpy);

        await increaseFakeTime(1);

        expect(capDiscoverySpy.lookup.mock.calls.length).toBe(1);
        await increaseFakeTime(discoveryQosWithShortTimers.discoveryRetryDelayMs - 2);

        capDiscoverySpy.lookup.mockReturnValue(Promise.resolve(secondLookupResult));
        expect(capDiscoverySpy.lookup.mock.calls.length).toBe(1);
        await increaseFakeTime(2);

        expect(capDiscoverySpy.lookup.mock.calls.length).toBe(2);
        await increaseFakeTime(1000);

        expect(onRejectedSpy).toHaveBeenCalled();
        expect(onFulfilledSpy).not.toHaveBeenCalled();
        expect(onRejectedSpy.mock.calls[0][0] instanceof NoCompatibleProviderFoundException).toBeTruthy();
        // discoverVersion should contain all not matching entries of only the last(!) lookup
        const discoveredVersions = onRejectedSpy.mock.calls[0][0].discoveredVersions;
        expect(discoveredVersions).toContain(discoveryEntryWithMajor47AndMinor0.providerVersion);
        expect(discoveredVersions).not.toContain(discoveryEntryWithMajor47AndMinor1.providerVersion);
        expect(discoveredVersions).not.toContain(discoveryEntryWithMajor47AndMinor2.providerVersion);
        expect(discoveredVersions).not.toContain(discoveryEntryWithMajor47AndMinor3.providerVersion);
        expect(discoveredVersions).toContain(discoveryEntryWithMajor48AndMinor2.providerVersion);
    });

    it("timeouts when no retry is possible any more", async () => {
        const onRejectedSpy = jest.fn();
        arbitrator
            .startArbitration({
                domains: [domain],
                interfaceName,
                discoveryQos,
                proxyVersion: new Version({ majorVersion: 47, minorVersion: 11 })
            })
            .catch(onRejectedSpy);

        // stop 1ms before last possible retry
        const delay = discoveryQos.discoveryTimeoutMs - discoveryQos.discoveryRetryDelayMs - 1;
        await increaseFakeTime(delay);
        expect(onRejectedSpy).not.toHaveBeenCalled();

        // largest amount of possible retries has passed
        await increaseFakeTime(discoveryQos.discoveryRetryDelayMs);

        expect(onRejectedSpy).toHaveBeenCalled();
        expect(onRejectedSpy.mock.calls.slice(-1)[0][0] instanceof DiscoveryException).toBeTruthy();
    });

    it("timeouts after the given discoveryTimeoutMs when capabilityDiscoveryStub throws exceptions", async () => {
        const onRejectedSpy = jest.fn();
        const fakeError = new Error("simulate discovery exception");
        capDiscoverySpy.lookup.mockReturnValue(Promise.reject(fakeError));
        discoveryQos.discoveryTimeoutMs = 500;
        discoveryQos.discoveryRetryDelayMs = 30;

        arbitrator
            .startArbitration({
                domains: [domain],
                interfaceName,
                discoveryQos,
                proxyVersion: new Version({ majorVersion: 47, minorVersion: 11 })
            })
            .catch((error: any) => {
                onRejectedSpy(error);
            });

        const checkpointMs = discoveryQos.discoveryRetryDelayMs * 5;
        // let checkpoint ms pass
        await increaseFakeTime(checkpointMs);
        expect(onRejectedSpy).not.toHaveBeenCalled();
        // let discoveryTimeoutMs pass
        await increaseFakeTime(discoveryQos.discoveryTimeoutMs - checkpointMs);

        expect(onRejectedSpy).toHaveBeenCalled();
        expect(onRejectedSpy.mock.calls.slice(-1)[0][0] instanceof DiscoveryException).toBeTruthy();
        expect(onRejectedSpy.mock.calls.slice(-1)[0][0].detailMessage).toMatch(fakeError.message);
    });

    async function testDiscoveryRetryByDiscoveryError(discoveryError: DiscoveryError, expectedRetry: boolean) {
        const expectedCalls = expectedRetry ? 2 : 1;
        const onRejectedSpy = jest.fn();
        const fakeError = new ApplicationException({ detailMessage: "test", error: discoveryError });
        capDiscoverySpy.lookup.mockRejectedValue(fakeError);

        arbitrator
            .startArbitration({
                domains: [domain],
                interfaceName,
                discoveryQos,
                proxyVersion: new Version({ majorVersion: 47, minorVersion: 11 })
            })
            .catch(onRejectedSpy);

        await testUtil.multipleSetImmediate();
        await increaseFakeTime(discoveryQos.discoveryTimeoutMs);

        expect(capDiscoverySpy.lookup).toHaveBeenCalledTimes(expectedCalls);
        expect(onRejectedSpy).toHaveBeenCalled();
        expect(onRejectedSpy.mock.calls.slice(-1)[0][0] instanceof DiscoveryException).toBeTruthy();
        expect(onRejectedSpy.mock.calls.slice(-1)[0][0].detailMessage).toMatch(discoveryError.name);
    }
    [DiscoveryError.INTERNAL_ERROR, DiscoveryError.INVALID_GBID, DiscoveryError.UNKNOWN_GBID].forEach(
        discoveryError => {
            it(`won't retry lookup for DiscoveryError ${discoveryError.name}`, () => {
                return testDiscoveryRetryByDiscoveryError(discoveryError, false);
            });
        }
    );

    [DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS, DiscoveryError.NO_ENTRY_FOR_PARTICIPANT].forEach(discoveryError => {
        it(`retries lookup for DiscoveryError ${discoveryError.name}`, () => {
            return testDiscoveryRetryByDiscoveryError(discoveryError, true);
        });
    });

    it("reruns discovery for empty discovery results according to discoveryTimeoutMs and discoveryRetryDelayMs", async () => {
        //capDiscoverySpy.lookup.and.returnValue(Promise.resolve([]));
        capDiscoverySpy.lookup.mockImplementation(() => {
            return Promise.resolve([]);
        });
        arbitrator = new Arbitrator(capDiscoverySpy);

        expect(capDiscoverySpy.lookup).not.toHaveBeenCalled();
        jest.spyOn(discoveryQos, "arbitrationStrategy").mockReturnValue([]);
        let res: Function;
        const promise = new Promise(resolve => (res = resolve));

        arbitrator
            .startArbitration({
                domains: [domain],
                interfaceName,
                discoveryQos,
                proxyVersion: new Version({ majorVersion: 47, minorVersion: 11 })
            })
            .catch(() => {
                // startAbitration caught error [expected!]
                res();
            });

        await increaseFakeTime(1);

        for (let i = 1; i < nrTimes + 1; ++i) {
            await increaseFakeTime(discoveryQos.discoveryRetryDelayMs - 2);
            expect(capDiscoverySpy.lookup.mock.calls.length).toBe(i);
            await increaseFakeTime(2);
            expect(capDiscoverySpy.lookup.mock.calls.length).toBe(i + 1);
        }
        // this should trigger the done in the catch of startArbitration
        // above
        await increaseFakeTime(discoveryQos.discoveryTimeoutMs);
        await promise;
    });

    it("uses arbitration strategy and returns its results", async () => {
        // just return some object so that arbitration is successful and
        // arbitration strategy is called
        capDiscoverySpy.lookup.mockReturnValue(Promise.resolve(discoveryEntries));
        arbitrator = new Arbitrator(capDiscoverySpy);

        // spy on and instrument arbitrationStrategy to return discoveryEntries
        jest.spyOn(discoveryQos, "arbitrationStrategy").mockReturnValue(discoveryEntries);

        const discoveredEntries = await arbitrator.startArbitration({
            domains: [domain],
            interfaceName,
            discoveryQos,
            proxyVersion: new Version({ majorVersion: 47, minorVersion: 11 })
        });

        // the arbitrationStrategy was called with the discoveryEntries
        // returned by the discovery spy
        expect(discoveryQos.arbitrationStrategy).toHaveBeenCalled();
        expect(discoveryQos.arbitrationStrategy).toHaveBeenCalledWith(discoveryEntries);
        expect(discoveredEntries).toEqual(discoveryEntries);
        // increaseFakeTime: is required for test purpose to ensure the
        // resolve/reject callbacks are called
        increaseFakeTime(1);
    });

    it("is instantiable, of correct type and has all members", () => {
        expect(Arbitrator).toBeDefined();
        expect(typeof Arbitrator === "function").toBeTruthy();
        expect(arbitrator).toBeDefined();
        expect(arbitrator instanceof Arbitrator).toBeTruthy();
        expect(arbitrator.startArbitration).toBeDefined();
        expect(typeof arbitrator.startArbitration === "function").toBeTruthy();
    });

    function arbitratesCorrectly(settings: any) {
        staticArbitrationSettings.domains = settings.domains;
        staticArbitrationSettings.interfaceName = settings.interfaceName;
        staticArbitrationSettings.discoveryQos = new DiscoveryQos(
            UtilInternal.extend(staticArbitrationSettings.discoveryQos, {
                providerMustSupportOnChange: settings.providerMustSupportOnChange || false
            })
        );
        return arbitrator.startArbitration(staticArbitrationSettings).then(discoveredEntries => {
            expect(discoveredEntries).toEqual(settings.expected);
        });
    }

    it("arbitrates correctly static capabilities", async () => {
        jest.spyOn(discoveryQos, "arbitrationStrategy").mockImplementation((discoveredCaps: any) => {
            return discoveredCaps;
        });

        await arbitratesCorrectly({
            domains: ["myDomain"],
            interfaceName: "noneExistingInterface",
            expected: []
        });

        await arbitratesCorrectly({
            domains: ["noneExistingDomain"],
            interfaceName: "myInterface",
            expected: []
        });

        await arbitratesCorrectly({
            domains: ["myDomain"],
            interfaceName: "myInterface",
            expected: [capabilities[0], capabilities[1]]
        });

        await arbitratesCorrectly({
            domains: ["otherDomain"],
            interfaceName: "otherInterface",
            expected: [capabilities[2]]
        });

        await arbitratesCorrectly({
            domains: ["thirdDomain"],
            interfaceName: "otherInterface",
            expected: [capabilities[3]]
        });
    });

    it("Arbitrator supports discoveryQos.providerSupportsOnChange for static arbitration", async () => {
        jest.spyOn(discoveryQos, "arbitrationStrategy").mockImplementation((discoveredCaps: any) => {
            return discoveredCaps;
        });

        await arbitratesCorrectly({
            domains: ["myDomain"],
            interfaceName: "noneExistingInterface",
            providerMustSupportOnChange: true,
            expected: []
        });

        await arbitratesCorrectly({
            domains: ["noneExistingDomain"],
            interfaceName: "myInterface",
            providerMustSupportOnChange: true,
            expected: []
        });

        await arbitratesCorrectly({
            domains: ["myDomain"],
            interfaceName: "myInterface",
            providerMustSupportOnChange: false,
            expected: [capabilities[0], capabilities[1]]
        });

        await arbitratesCorrectly({
            domains: ["myDomain"],
            interfaceName: "myInterface",
            providerMustSupportOnChange: true,
            expected: [capabilities[0]]
        });

        await arbitratesCorrectly({
            domains: ["otherDomain"],
            interfaceName: "otherInterface",
            providerMustSupportOnChange: true,
            expected: [capabilities[2]]
        });

        await arbitratesCorrectly({
            domains: ["thirdDomain"],
            interfaceName: "otherInterface",
            providerMustSupportOnChange: true,
            expected: []
        });

        await arbitratesCorrectly({
            domains: ["thirdDomain"],
            interfaceName: "otherInterface",
            providerMustSupportOnChange: false,
            expected: [capabilities[3]]
        });
    });

    it("uses the provided arbitrationStrategy", async () => {
        jest.spyOn(discoveryQos, "arbitrationStrategy").mockReturnValue(discoveryEntries);

        const discoveredEntries = await arbitrator.startArbitration(staticArbitrationSettings);
        expect(discoveredEntries).toEqual(discoveryEntries);
        expect(discoveryQos.arbitrationStrategy).toHaveBeenCalledWith([capabilities[0], capabilities[1]]);
    });

    it("fails if arbitrationStrategy throws an exception", async () => {
        jest.spyOn(discoveryQos, "arbitrationStrategy").mockImplementation(() => {
            throw new Error("myError");
        });
        await expect(arbitrator.startArbitration(staticArbitrationSettings)).rejects.toBeInstanceOf(Error);
    });

    it("rejects pending arbitrations when shutting down", async () => {
        capDiscoverySpy.lookup.mockReturnValue(Promise.resolve([]));
        jest.spyOn(discoveryQos, "arbitrationStrategy").mockReturnValue([]);
        arbitrator = new Arbitrator(capDiscoverySpy);
        const onRejectedSpy = jest.fn();

        // start arbitration
        arbitrator
            .startArbitration({
                domains: [domain],
                interfaceName,
                discoveryQos,
                proxyVersion: new Version({ majorVersion: 47, minorVersion: 11 })
            })
            .catch(onRejectedSpy);
        arbitrator.shutdown();
        await testUtil.multipleSetImmediate();
        expect(onRejectedSpy).toHaveBeenCalled();
    });

    function arbitrationStrategyReturningFirstFoundEntry(
        capabilities: DiscoveryEntryWithMetaInfo[]
    ): DiscoveryEntryWithMetaInfo[] {
        const caps: DiscoveryEntryWithMetaInfo[] = [];

        if (!Array.isArray(capabilities)) {
            throw new Error("provided argument capabilities is not of type Array");
        }

        for (const capId in capabilities) {
            if (capabilities.hasOwnProperty(capId)) {
                caps.push(capabilities[capId]);
                return caps;
            }
        }
        return caps;
    }

    it("filters capabilities properly and in order by version and arbitration strategy", async () => {
        const qosParam = new CustomParameter({
            name: "keyword",
            value: "valid"
        });
        const specificProviderQos = new ProviderQos({
            customParameters: [qosParam],
            priority: 123,
            scope:
                discoveryQos.discoveryScope === DiscoveryScope.LOCAL_ONLY ? ProviderScope.LOCAL : ProviderScope.GLOBAL,
            supportsOnChangeSubscriptions: false
        });
        UtilInternal.extend(discoveryEntryWithMajor47AndMinor3, { qos: specificProviderQos });

        const discoveryEntriesWithDifferentProviderVersions = [
            discoveryEntryWithMajor47AndMinor0,
            discoveryEntryWithMajor47AndMinor3,
            discoveryEntryWithMajor48AndMinor2
        ];

        const arbitrationStrategy = arbitrationStrategyReturningFirstFoundEntry;

        // return discoveryEntries to check whether these are eventually
        // returned by the arbitrator
        capDiscoverySpy.lookup.mockResolvedValue(discoveryEntriesWithDifferentProviderVersions);
        arbitrator = new Arbitrator(capDiscoverySpy);

        const returnedDiscoveryEntries = await arbitrator.startArbitration({
            domains: [domain],
            interfaceName,
            discoveryQos: new DiscoveryQos(UtilInternal.extend(discoveryQos, { arbitrationStrategy })),
            proxyVersion: new Version({ majorVersion: 47, minorVersion: 3 })
        });

        expect(returnedDiscoveryEntries.length).toEqual(1);
        expect(returnedDiscoveryEntries).not.toContain(discoveryEntryWithMajor47AndMinor0);
        expect(returnedDiscoveryEntries).toContain(discoveryEntryWithMajor47AndMinor3);
        expect(returnedDiscoveryEntries).not.toContain(discoveryEntryWithMajor48AndMinor2);
    });

    it("filters capabilities properly by supportOnChangeSubscriptions and arbitration strategy", async () => {
        const qosParam = new CustomParameter({
            name: "keyword",
            value: "valid"
        });
        const specificProviderQos = new ProviderQos({
            customParameters: [qosParam],
            priority: 123,
            scope:
                discoveryQos.discoveryScope === DiscoveryScope.LOCAL_ONLY ? ProviderScope.LOCAL : ProviderScope.GLOBAL,
            supportsOnChangeSubscriptions: false
        });

        UtilInternal.extend(discoveryEntryWithMajor47AndMinor0, { qos: specificProviderQos });
        UtilInternal.extend(discoveryEntryWithMajor48AndMinor2, { qos: specificProviderQos });

        const discoveryEntries = [
            discoveryEntryWithMajor47AndMinor0,
            discoveryEntryWithMajor47AndMinor3,
            discoveryEntryWithMajor48AndMinor2
        ];

        const arbitrationStrategy = arbitrationStrategyReturningFirstFoundEntry; //.bind(undefined, "valid");

        // return discoveryEntries to check whether these are eventually
        // returned by the arbitrator
        capDiscoverySpy.lookup.mockResolvedValue(discoveryEntries);
        arbitrator = new Arbitrator(capDiscoverySpy);

        const returnedDiscoveryEntries = await arbitrator.startArbitration({
            domains: [domain],
            interfaceName,
            discoveryQos: new DiscoveryQos(
                UtilInternal.extend(discoveryQos, { providerMustSupportOnChange: true, arbitrationStrategy })
            ),
            proxyVersion: new Version({ majorVersion: 47, minorVersion: 0 })
        });

        expect(returnedDiscoveryEntries.length).toEqual(1);
        expect(returnedDiscoveryEntries).not.toContain(discoveryEntryWithMajor47AndMinor0);
        expect(returnedDiscoveryEntries).toContain(discoveryEntryWithMajor47AndMinor3);
        expect(returnedDiscoveryEntries).not.toContain(discoveryEntryWithMajor48AndMinor2);
    });
});
