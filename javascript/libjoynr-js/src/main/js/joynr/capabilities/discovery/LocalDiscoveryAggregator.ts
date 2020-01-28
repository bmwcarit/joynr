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

class LocalDiscoveryAggregator implements DiscoveryStub {
    private discoveryProxy!: DiscoveryProxy;
    public constructor() {}

    public setDiscoveryProxy(discoveryProxy: DiscoveryProxy): void {
        this.discoveryProxy = discoveryProxy;
    }

    public lookupByParticipantId(
        participantId: string,
        discoveryQos: DiscoveryQos,
        gbids: string[]
    ): Promise<DiscoveryEntryWithMetaInfo> {
        return this.discoveryProxy
            .lookup({
                participantId,
                discoveryQos,
                gbids
            })
            .then(opArgs => {
                return opArgs.result;
            });
    }

    public lookup(
        domains: string[],
        interfaceName: string,
        discoveryQos: DiscoveryQos,
        gbids: string[]
    ): Promise<DiscoveryEntryWithMetaInfo[]> {
        // eslint-disable-next-line promise/no-nesting
        return this.discoveryProxy
            .lookup({
                domains,
                interfaceName,
                discoveryQos,
                gbids
            })
            .then(opArgs => {
                return opArgs.result;
            });
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
