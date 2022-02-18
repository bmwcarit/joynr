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

import joynr = require("joynr");

import RadioProxy from "../../../generated/joynr/vehicle/RadioProxy";
import TestWithVersionProvider from "../../../generated/joynr/tests/TestWithVersionProvider";
import TestWithVersionProxy from "../../../generated/joynr/tests/TestWithVersionProxy";
import * as IntegrationUtils from "../IntegrationUtils";
import provisioning from "../../../resources/joynr/provisioning/provisioning_cc";
import DiscoveryException from "../../../../main/js/joynr/exceptions/DiscoveryException";
import DiscoveryQos from "../../../../main/js/joynr/proxy/DiscoveryQos";
import DiscoveryScope from "../../../../main/js/generated/joynr/types/DiscoveryScope";
import InProcessRuntime = require("joynr/joynr/start/InProcessRuntime");
import { FixedParticipant } from "../../../../main/js/joynr/types/ArbitrationStrategyCollection";
import { FIXED_PARTICIPANT_PARAMETER } from "../../../../main/js/joynr/types/ArbitrationConstants";

describe("libjoynr-js.integration.localDiscoveryTest", () => {
    let provisioningSuffix: any;
    let domain: any;
    let childId: any;
    class MyTestWithVersionProvider {}

    afterEach(async () => {
        await IntegrationUtils.shutdownLibjoynr();
    });

    beforeEach(async () => {
        provisioningSuffix = `LocalDiscoveryTest-${Date.now()}`;
        domain = provisioningSuffix;

        (provisioning as any).channelId = provisioningSuffix;
        // @ts-ignore
        joynr.loaded = false;
        joynr.selectRuntime(InProcessRuntime);

        await joynr.load(provisioning as any);
        IntegrationUtils.initialize();
    });

    async function registerGlobalDiscoveryEntry() {
        const newChildId = await IntegrationUtils.initializeChildProcess(
            "TestEnd2EndCommProviderProcess",
            provisioningSuffix,
            domain
        );
        childId = newChildId;
        return IntegrationUtils.startChildProcess(childId);
    }

    function unregisterGlobalDiscoveryEntry() {
        return IntegrationUtils.shutdownChildProcess(childId);
    }

    function buildProxyForGlobalDiscoveryEntry() {
        return joynr.proxyBuilder.build(RadioProxy, {
            domain,
            messagingQos: new joynr.messaging.MessagingQos(),
            discoveryQos: new DiscoveryQos()
        });
    }

    it("arbitration strategy FixedParticipant", async () => {
        const providerQos = new joynr.types.ProviderQos({
            customParameters: [],
            priority: Date.now(),
            scope: joynr.types.ProviderScope.LOCAL,
            supportsOnChangeSubscriptions: true
        });

        const testWithVersionProviderImpl = new MyTestWithVersionProvider();
        const testWithVersionProvider = joynr.providerBuilder.build(
            TestWithVersionProvider,
            testWithVersionProviderImpl
        );

        const participantId = await joynr.registration.registerProvider(domain, testWithVersionProvider, providerQos);

        const proxy1 = await joynr.proxyBuilder.build(TestWithVersionProxy, {
            domain,
            messagingQos: new joynr.messaging.MessagingQos(),
            discoveryQos: new DiscoveryQos({
                discoveryTimeoutMs: 1000,
                arbitrationStrategy: FixedParticipant,
                discoveryScope: DiscoveryScope.LOCAL_ONLY,
                additionalParameters: { [FIXED_PARTICIPANT_PARAMETER]: participantId }
            })
        });

        expect((proxy1.providerDiscoveryEntry as any).participantId).toBeDefined();
        expect((proxy1.providerDiscoveryEntry as any).participantId).toEqual(participantId);

        try {
            await joynr.proxyBuilder.build(TestWithVersionProxy, {
                domain,
                messagingQos: new joynr.messaging.MessagingQos(),
                discoveryQos: new DiscoveryQos({
                    discoveryTimeoutMs: 1000,
                    arbitrationStrategy: FixedParticipant,
                    discoveryScope: DiscoveryScope.LOCAL_ONLY,
                    additionalParameters: { [FIXED_PARTICIPANT_PARAMETER]: "otherParticipantId" }
                })
            });
            throw new Error("unexpected success");
        } catch (e) {
            expect(e instanceof DiscoveryException).toBe(true);
        }

        const proxy2 = await joynr.proxyBuilder.build(TestWithVersionProxy, {
            domain,
            messagingQos: new joynr.messaging.MessagingQos(),
            discoveryQos: new DiscoveryQos({
                discoveryTimeoutMs: 1000,
                discoveryScope: DiscoveryScope.LOCAL_ONLY,
                additionalParameters: { [FIXED_PARTICIPANT_PARAMETER]: "otherParticipantId" }
            })
        });
        expect(proxy1.providerDiscoveryEntry).toEqual(proxy2.providerDiscoveryEntry);

        await joynr.registration.unregisterProvider(domain, testWithVersionProvider);
    });

    it("local discovery entry is forwarded to proxy", async () => {
        const providerQos = new joynr.types.ProviderQos({
            customParameters: [],
            priority: Date.now(),
            scope: joynr.types.ProviderScope.GLOBAL,
            supportsOnChangeSubscriptions: true
        });

        const testWithVersionProviderImpl = new MyTestWithVersionProvider();
        const testWithVersionProvider = joynr.providerBuilder.build(
            TestWithVersionProvider,
            testWithVersionProviderImpl
        );

        await joynr.registration.registerProvider(domain, testWithVersionProvider, providerQos);

        const testWithVersionProxy = await joynr.proxyBuilder.build(TestWithVersionProxy, {
            domain,
            messagingQos: new joynr.messaging.MessagingQos(),
            discoveryQos: new DiscoveryQos()
        });

        expect((testWithVersionProxy.providerDiscoveryEntry as any).isLocal).toBeDefined();
        expect((testWithVersionProxy.providerDiscoveryEntry as any).isLocal).toBe(true);
        await joynr.registration.unregisterProvider(domain, testWithVersionProvider);
    });

    it("global discovery entry is forwarded to proxy", async () => {
        try {
            await registerGlobalDiscoveryEntry();
            const radioProxy = await buildProxyForGlobalDiscoveryEntry();
            expect((radioProxy.providerDiscoveryEntry as any).isLocal).toBeDefined();
            expect((radioProxy.providerDiscoveryEntry as any).isLocal).toBe(false);
            await unregisterGlobalDiscoveryEntry();
        } catch (e) {
            await unregisterGlobalDiscoveryEntry()
                .then(() => {
                    throw e;
                })
                .catch((e2: any) => {
                    throw new Error(`Error1: ${e}\n Error2: ${e2}`);
                });
        }
    });

    it("cached global discovery entry is forwarded to proxy", async () => {
        try {
            await registerGlobalDiscoveryEntry();
            await buildProxyForGlobalDiscoveryEntry();
            const radioProxy = await buildProxyForGlobalDiscoveryEntry();
            expect((radioProxy.providerDiscoveryEntry as any).isLocal).toBeDefined();
            expect((radioProxy.providerDiscoveryEntry as any).isLocal).toBe(false);
            await unregisterGlobalDiscoveryEntry();
        } catch (e) {
            await unregisterGlobalDiscoveryEntry()
                .then(() => {
                    throw e;
                })
                .catch((e2: any) => {
                    throw new Error(`Error1: ${e}\n Error2: ${e2}`);
                });
        }
    });
}); // describe
