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
import DiscoveryEntry from "../../generated/joynr/types/DiscoveryEntry";

import GlobalDiscoveryEntry from "../../generated/joynr/types/GlobalDiscoveryEntry";
import DiscoveryEntryWithMetaInfo, {
    DiscoveryEntryWithMetaInfoMembers
} from "../../generated/joynr/types/DiscoveryEntryWithMetaInfo";

/**
 * This method transforms a capabilityInformation into an object of type "DiscoveryEntry"
 *
 * @param capabilityInformation the object to be transformed
 *
 * @returns the transformed object
 */
export function toDiscoveryEntry(capabilityInformation: any): DiscoveryEntry {
    return new DiscoveryEntry({
        providerVersion: capabilityInformation.providerVersion,
        domain: capabilityInformation.domain,
        interfaceName: capabilityInformation.interfaceName,
        qos: capabilityInformation.providerQos,
        participantId: capabilityInformation.participantId,
        publicKeyId: capabilityInformation.publicKeyId,
        lastSeenDateMs: Date.now(),
        expiryDateMs: undefined as any
    });
}

/**
 * This method transforms an array of capabilityInformations into an array of objects
 * of type DiscoveryEntry
 *
 * @param capabilityInformations array of capability information
 *
 * @returns array of transformed objects of type DiscoveryEntry
 */
export function toDiscoveryEntries(capabilityInformations: any[]): DiscoveryEntry[] {
    const discoveryEntries = [];
    if (capabilityInformations) {
        for (let i = 0; i < capabilityInformations.length; i++) {
            discoveryEntries.push(toDiscoveryEntry(capabilityInformations[i]));
        }
    }
    return discoveryEntries;
}

/**
 * This method transforms a DiscoveryEntry into a GlobalDiscoveryEntry
 * with the given address.
 *
 * @param discoveryEntry the DiscoveryEntry to be transformed
 * @param address the address to be used for the GlobalDiscoveryEntry
 *
 * @returns global DiscoveryEntry with provided address
 */
export function discoveryEntry2GlobalDiscoveryEntry(
    discoveryEntry: DiscoveryEntry,
    address: any
): GlobalDiscoveryEntry {
    return new GlobalDiscoveryEntry({
        providerVersion: discoveryEntry.providerVersion,
        domain: discoveryEntry.domain,
        interfaceName: discoveryEntry.interfaceName,
        participantId: discoveryEntry.participantId,
        qos: discoveryEntry.qos,
        lastSeenDateMs: discoveryEntry.lastSeenDateMs,
        expiryDateMs: discoveryEntry.expiryDateMs,
        publicKeyId: discoveryEntry.publicKeyId,
        address: JSON.stringify(address)
    });
}

/**
 * This method transforms a DiscoveryEntry into a DiscoveryEntryWithMetaInfo
 * with the given isLocal value.
 *
 * @param isLocal true, if it is a local DiscoveryEntry, false otherwise
 * @param discoveryEntry the DiscoveryEntry to be transformed
 *
 * @returns DiscoveryEntryWithMetaInfo with the given isLocal value
 */
export function convertToDiscoveryEntryWithMetaInfo(
    isLocal: boolean,
    discoveryEntry: DiscoveryEntry
): DiscoveryEntryWithMetaInfo {
    ((discoveryEntry as any) as DiscoveryEntryWithMetaInfoMembers).isLocal = isLocal;
    return new DiscoveryEntryWithMetaInfo((discoveryEntry as any) as DiscoveryEntryWithMetaInfoMembers);
}

/**
 * This method transforms an array of DiscoveryEntries into an array of objects
 * of type DiscoveryEntryWithMetaInfo
 *
 * @param isLocal true, if the DiscoveryEntries are local DiscoveryEntry, false otherwise
 * @param discoveryEntries array of DiscoveryEntries
 *
 * @returns array of transformed objects of type DiscoveryEntryWithMetaInfo with the given isLocal value
 */
export function convertToDiscoveryEntryWithMetaInfoArray(isLocal: boolean, discoveryEntries: any[]): any[] {
    const result = [];
    if (discoveryEntries) {
        for (let i = 0; i < discoveryEntries.length; i++) {
            result.push(convertToDiscoveryEntryWithMetaInfo(isLocal, discoveryEntries[i]));
        }
    }
    return result;
}

/**
 * This method generates a key of the currently used format for storage of
 * a capability's participantId in the ParticipantIdStorage
 *
 * @param domain the domain of the capability that is to be stored
 * @param interfaceName the interface of the capability that is to be stored
 * @param majorVersion the major Version of the capability that is to be stored
 *
 * @returns a key to store the participantId with
 */
export function generateParticipantIdStorageKey(domain: string, interfaceName: string, majorVersion: number): string {
    return `joynr.participant.${domain}.${interfaceName}.v${majorVersion}`;
}
