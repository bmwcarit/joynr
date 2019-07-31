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

import ProxyBuilder from "../../../../main/js/joynr/proxy/ProxyBuilder";
import DiscoveryQos from "../../../../main/js/joynr/proxy/DiscoveryQos";
import MessagingQos from "../../../../main/js/joynr/messaging/MessagingQos";
import ProviderQos from "../../../../main/js/generated/joynr/types/ProviderQos";
import ProviderScope from "../../../../main/js/generated/joynr/types/ProviderScope";
import DiscoveryEntryWithMetaInfo from "../../../../main/js/generated/joynr/types/DiscoveryEntryWithMetaInfo";
import * as ArbitrationStrategyCollection from "../../../../main/js/joynr/types/ArbitrationStrategyCollection";
import DiscoveryScope from "../../../../main/js/generated/joynr/types/DiscoveryScope";
import Version from "../../../../main/js/generated/joynr/types/Version";
import RadioProxy from "../../../generated/joynr/vehicle/RadioProxy";
import TypeRegistry from "../../../../main/js/joynr/start/TypeRegistry";
const typeRegistry = new TypeRegistry();

let interfaceName = "io/joynr/apps/radio";

describe("libjoynr-js.joynr.proxy.ProxyBuilder", () => {
    let proxyBuilder: any;
    let domain: any;
    let arbitratorSpy: any;
    let discoveryQos: any;
    let settings: any;
    let capInfo: any;
    let arbitratedCaps: any;
    let messagingQos: any;
    let messageRouterSpy: any;
    let libjoynrMessagingAddress: any;

    beforeEach(done => {
        domain = "myDomain";
        interfaceName = "vehicle/Radio";
        discoveryQos = new DiscoveryQos({
            discoveryTimeoutMs: 5000,
            discoveryRetryDelayMs: 900,
            arbitrationStrategy: ArbitrationStrategyCollection.Nothing,
            cacheMaxAgeMs: 0,
            discoveryScope: DiscoveryScope.LOCAL_THEN_GLOBAL,
            additionalParameters: {}
        });
        messagingQos = new MessagingQos();
        settings = {
            domain,
            discoveryQos,
            messagingQos,
            staticArbitration: false
        };

        capInfo = new DiscoveryEntryWithMetaInfo({
            providerVersion: new Version({ majorVersion: 47, minorVersion: 11 }),
            domain,
            interfaceName,
            participantId: "myParticipantId",
            qos: new ProviderQos({
                customParameters: [],
                priority: 1,
                scope: ProviderScope.GLOBAL,
                supportsOnChangeSubscriptions: true
            }),
            lastSeenDateMs: Date.now(),
            expiryDateMs: Date.now() + 60000,
            publicKeyId: "",
            isLocal: false
        });

        arbitratedCaps = [capInfo];

        arbitratorSpy = {
            startArbitration: jest.fn()
        };
        messageRouterSpy = {
            addNextHop: jest.fn(),
            setToKnown: jest.fn()
        };
        libjoynrMessagingAddress = {
            key: "libjoynrMessagingAddress"
        };

        const resolvedPromise = Promise.resolve(arbitratedCaps);
        arbitratorSpy.startArbitration.mockReturnValue(resolvedPromise);
        messageRouterSpy.addNextHop.mockReturnValue(resolvedPromise);
        proxyBuilder = new ProxyBuilder(
            {
                arbitrator: arbitratorSpy,
                requestReplyManager: {} as any,
                subscriptionManager: {} as any,
                publicationManager: {} as any
            },
            {
                messageRouter: messageRouterSpy,
                libjoynrMessagingAddress,
                typeRegistry
            }
        );
        done();
    });

    it("is defined and of correct type", done => {
        expect(proxyBuilder).toBeDefined();
        expect(typeof proxyBuilder.build === "function").toBe(true);
        done();
    });

    it("throws exceptions upon missing or wrongly typed arguments", async () => {
        // settings is undefined
        await expect(proxyBuilder.build(RadioProxy)).rejects.toBeInstanceOf(Error);
        // settings is not of type object
        await expect(proxyBuilder.build(RadioProxy, "notObject")).rejects.toBeInstanceOf(Error);
        // domain is undefined
        await expect(
            proxyBuilder.build(RadioProxy, {
                discoveryQos: new DiscoveryQos(),
                messagingQos: new MessagingQos()
            })
        ).rejects.toBeInstanceOf(Error);
        // domain is not of type string
        await expect(
            proxyBuilder.build(RadioProxy, {
                domain: 1234,
                discoveryQos: new DiscoveryQos(),
                messagingQos: new MessagingQos()
            })
        ).rejects.toBeInstanceOf(Error);
        // discoveryQos is not of type DiscoveryQos
        await expect(
            proxyBuilder.build(RadioProxy, {
                domain: "",
                discoveryQos: {},
                messagingQos: new MessagingQos()
            })
        ).resolves.toBeDefined();
        // messagingQos is is not of type MessagingQos
        await expect(
            proxyBuilder.build(RadioProxy, {
                domain: "",
                discoveryQos: new DiscoveryQos(),
                messagingQos: {}
            })
        ).resolves.toBeDefined();
        // discoveryQos is missing
        await expect(
            proxyBuilder.build(RadioProxy, {
                domain: "",
                messagingQos: new MessagingQos()
            })
        ).resolves.toBeDefined();
        // messagingQos is missing
        await expect(
            proxyBuilder.build(RadioProxy, {
                domain: "",
                discoveryQos: new DiscoveryQos()
            })
        ).resolves.toBeDefined();
        // messagingQos and discoveryQos are missing
        await expect(
            proxyBuilder.build(RadioProxy, {
                domain: ""
            })
        ).resolves.toBeDefined();
        // ok
        await expect(
            proxyBuilder.build(RadioProxy, {
                domain: "",
                discoveryQos: new DiscoveryQos(),
                messagingQos: new MessagingQos()
            })
        ).resolves.toBeDefined();
    });

    it("does not throw", () => {
        expect(() => {
            proxyBuilder.build(RadioProxy, settings);
        }).not.toThrow();
    });

    it("calls arbitrator with correct arguments", async () => {
        await proxyBuilder.build(RadioProxy, settings);

        expect(arbitratorSpy.startArbitration).toHaveBeenCalledWith({
            domains: [settings.domain],
            interfaceName,
            discoveryQos: settings.discoveryQos,
            staticArbitration: settings.staticArbitration,
            proxyVersion: new Version({ majorVersion: 47, minorVersion: 11 })
        });
    });

    it("returned promise is rejected with error", async () => {
        const error = new Error("MyError");

        arbitratorSpy.startArbitration.mockReturnValue(Promise.reject(error));
        proxyBuilder = new ProxyBuilder(
            {
                arbitrator: arbitratorSpy
            } as any,
            {} as any
        );
        await expect(proxyBuilder.build(RadioProxy, settings)).rejects.toBeInstanceOf(Error);
    });

    it("returned promise is resolved with proxy object with injected providerParticipantId", async () => {
        const proxy = await proxyBuilder.build(RadioProxy, settings);
        expect(proxy.providerDiscoveryEntry).toEqual(arbitratedCaps[0]);
    });

    it("adds a routing table entry for proxy and knows provider", async () => {
        const proxy = await proxyBuilder.build(RadioProxy, settings);
        expect(proxy.providerDiscoveryEntry).toEqual(arbitratedCaps[0]);
        expect(typeof messageRouterSpy.addNextHop.mock.calls.slice(-1)[0][0] === "string").toBeTruthy();
        expect(messageRouterSpy.setToKnown.mock.calls.slice(-1)[0][0]).toEqual(arbitratedCaps[0].participantId);
        expect(messageRouterSpy.addNextHop.mock.calls.slice(-1)[0][1]).toEqual(libjoynrMessagingAddress);
        expect(messageRouterSpy.addNextHop.mock.calls.slice(-1)[0][2]).toEqual(true);
    });
});
