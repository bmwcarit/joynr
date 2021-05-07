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

import DiscoveryProxy from "../../../generated/joynr/system/DiscoveryProxy";
import * as DiscoveryEntry from "../../../generated/joynr/types/DiscoveryEntry";
import * as DiscoveryEntryWithMetaInfo from "../../../generated/joynr/types/DiscoveryEntryWithMetaInfo";
import * as DiscoveryQos from "../../../generated/joynr/types/DiscoveryQos";
import { DiscoveryStub } from "../interface/DiscoveryStub";
import MessagingQos from "../../messaging/MessagingQos";

class LocalDiscoveryAggregator implements DiscoveryStub {
    private discoveryProxy!: DiscoveryProxy;
    private defaultMessagingQos!: MessagingQos;
    // Epsilon is added to be sure that the proxy call `lookup` gets results, whatever it is rather than timing out with
    // JoynrTimeoutException. By adding this epsilon, we avoid race condition between local expiration timer and the
    // delivery of the reply.
    private epsilonMs: number = 10000;
    public constructor() {}

    public setDiscoveryProxy(discoveryProxy: DiscoveryProxy): void {
        this.discoveryProxy = discoveryProxy;
        this.defaultMessagingQos = this.discoveryProxy.messagingQos;
    }

    private setDiscoveryProxyTtl(ttl: number): void {
        const qos = new MessagingQos(this.defaultMessagingQos);
        qos.ttl = ttl;
        this.discoveryProxy.messagingQos = qos;
    }

    private restoreDefaultMessagingQos(): void {
        this.discoveryProxy.messagingQos = this.defaultMessagingQos;
    }

    public lookupByParticipantId(
        participantId: string,
        discoveryQos: DiscoveryQos,
        gbids: string[]
    ): Promise<DiscoveryEntryWithMetaInfo> {
        // remaining discovery time + epsilon
        const ttl = discoveryQos.discoveryTimeout + this.epsilonMs;
        this.setDiscoveryProxyTtl(ttl);
        const promise: Promise<DiscoveryEntryWithMetaInfo> = this.discoveryProxy
            .lookup({
                participantId,
                discoveryQos,
                gbids
            })
            .then(opArgs => {
                return opArgs.result;
            });
        this.restoreDefaultMessagingQos();
        return promise;
    }

    public lookup(
        domains: string[],
        interfaceName: string,
        discoveryQos: DiscoveryQos,
        gbids: string[]
    ): Promise<DiscoveryEntryWithMetaInfo[]> {
        // remaining discovery time + epsilon
        const ttl = discoveryQos.discoveryTimeout + this.epsilonMs;
        this.setDiscoveryProxyTtl(ttl);
        const promise: Promise<DiscoveryEntryWithMetaInfo[]> = this.discoveryProxy
            .lookup({
                domains,
                interfaceName,
                discoveryQos,
                gbids
            })
            .then(opArgs => {
                return opArgs.result;
            });
        this.restoreDefaultMessagingQos();
        return promise;
    }

    public add(discoveryEntry: DiscoveryEntry, awaitGlobalRegistration: boolean, gbids: string[]): Promise<void> {
        return this.discoveryProxy.add({
            discoveryEntry,
            awaitGlobalRegistration,
            gbids
        });
    }
    public addToAll(discoveryEntry: DiscoveryEntry, awaitGlobalRegistration: boolean): Promise<void> {
        return this.discoveryProxy.addToAll({
            discoveryEntry,
            awaitGlobalRegistration
        });
    }
    public remove(participantId: string): Promise<void> {
        return this.discoveryProxy.remove({
            participantId
        });
    }
}

export = LocalDiscoveryAggregator;
