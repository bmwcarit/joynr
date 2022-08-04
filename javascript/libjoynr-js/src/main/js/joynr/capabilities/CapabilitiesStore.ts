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

import * as DiscoveryEntry from "../../generated/joynr/types/DiscoveryEntry";
import * as Typing from "../util/Typing";

/**
 * Private function. This function returns the hashCode of the given discovery entry, this should uniquely identify a
 * discovery entry within the capability directory.
 *
 * @param discoveryEntry - discovery entry
 */
function hashCode(discoveryEntry: DiscoveryEntry): string {
    return (
        discoveryEntry.domain +
        discoveryEntry.interfaceName +
        discoveryEntry.participantId +
        discoveryEntry.providerVersion.majorVersion +
        discoveryEntry.providerVersion.minorVersion +
        discoveryEntry.publicKeyId
    );
}

/**
 * Private function. Returns a key for indexing by the domain, interfaceName and providerQos.
 *
 * @param domain - the domain
 * @param interfaceName - the interface name
 * @returns the unique key for the domain and interface name
 */
function getDomainInterfaceNameKey(domain: string, interfaceName: string): string {
    return domain + interfaceName;
}

class CapabilitiesStore {
    private registeredCapabilitiesTime: Record<string, number> = {};

    // discovery entry hashCode -> Array of Capability Information
    private discoveryEntryStore: Record<string, any> = {};

    // domain + interface -> Array of GlobalDiscoveryEntry
    private discoveryEntryStoreByDomainInterfaceName: Record<string, any[]> = {};

    // participantId -> Array of Capability Information
    private discoveryEntryStoreByParticipantId: Record<string, any> = {};

    /**
     * The Local Capabilities Storage
     *
     * @constructor
     *
     * @classdesc
     * The <code>CapabilitiesStore</code> is a joynr internal interface. When a provider that is registered with joynr, the framework creates an
     * entry for that provider in the capabilities directory. These <code>{@link DiscoveryEntry}</code> entries contain access
     * information as well as supported Qos. The information is later used in the arbitration process to pick a provider for a proxy.
     *
     * @param initialCapabilities
     */
    public constructor(initialCapabilities?: DiscoveryEntry[]) {
        if (initialCapabilities && initialCapabilities.length > 0) {
            this.add({
                discoveryEntries: initialCapabilities
            });
        }
    }

    /**
     * Unregisters a discovery entry from the store.
     *
     * @param participantId - the participant ID uniquely identifying the discovery entry to be removed
     */
    private removeDiscoveryEntryFromStore(participantId: string): void {
        const discoveryEntry = this.discoveryEntryStoreByParticipantId[participantId];

        // unregister by participant id
        delete this.discoveryEntryStoreByParticipantId[participantId];

        if (discoveryEntry !== undefined) {
            // unregister by domain and interface
            const domainInterfaceKey = getDomainInterfaceNameKey(discoveryEntry.domain, discoveryEntry.interfaceName);
            this.discoveryEntryStoreByDomainInterfaceName[
                domainInterfaceKey
            ] = this.discoveryEntryStoreByDomainInterfaceName[domainInterfaceKey].filter(cap => {
                if (
                    cap.interfaceName === discoveryEntry.interfaceName &&
                    cap.domain === discoveryEntry.domain &&
                    cap.participantId === discoveryEntry.participantId
                ) {
                    return false;
                }
                return true;
            });

            // unregister master store
            const key = hashCode(discoveryEntry);
            delete this.discoveryEntryStore[key];
        }
    }

    /**
     * Registers discovery entry to the store.
     *
     * @param discoveryEntry - the capability to register
     */
    private addDiscoveryEntryToStore(discoveryEntry: DiscoveryEntry): void {
        let entryFound = false,
            entryId,
            entry;
        const discoveryEntryKey = hashCode(discoveryEntry);

        // master store, storing key to actual discovery entry object
        const isAlreadyThere = this.discoveryEntryStore[discoveryEntryKey];
        if (isAlreadyThere !== undefined) {
            this.removeDiscoveryEntryFromStore(isAlreadyThere);
        }
        this.discoveryEntryStore[discoveryEntryKey] = discoveryEntry;
        this.registeredCapabilitiesTime[discoveryEntryKey] = Date.now();

        // by participant id
        this.discoveryEntryStoreByParticipantId[discoveryEntry.participantId] = discoveryEntry;

        // by domain interface and provider qos
        const domainInterfaceKey = getDomainInterfaceNameKey(discoveryEntry.domain, discoveryEntry.interfaceName);
        if (this.discoveryEntryStoreByDomainInterfaceName[domainInterfaceKey] === undefined) {
            this.discoveryEntryStoreByDomainInterfaceName[domainInterfaceKey] = [];
        }

        const discoveryEntries = this.discoveryEntryStoreByDomainInterfaceName[domainInterfaceKey];

        for (entryId in discoveryEntries) {
            if (Object.prototype.hasOwnProperty.call(discoveryEntries, entryId) && !entryFound) {
                entry = discoveryEntries[entryId];
                if (entry.participantId === discoveryEntry.participantId) {
                    entryFound = true;
                }
            }
        }

        if (!entryFound) {
            this.discoveryEntryStoreByDomainInterfaceName[domainInterfaceKey].push(discoveryEntry);
        }
    }

    /**
     * Registers either a single discovery entry or a list of discovery entry to the store.
     *
     * @param settings - an object containing the required parameters
     * @param settings.discoveryEntries - an array of discovery entries to register
     * @param settings.discoveryEntry a single discovery entry to register (only if settings.discoveryEntries is not supplied)
     * @param settings.remote true if the provided entry/entries are gathered from remote
     *
     * @returns returns this for a fluent interface
     */
    public add(settings: {
        discoveryEntries?: DiscoveryEntry[];
        discoveryEntry?: DiscoveryEntry;
        remote?: boolean;
    }): CapabilitiesStore {
        if (settings.discoveryEntries !== undefined && Typing.getObjectType(settings.discoveryEntries) === "Array") {
            for (let i = 0; i < settings.discoveryEntries.length; i++) {
                this.addDiscoveryEntryToStore(settings.discoveryEntries[i]);
            }
        } else if (settings.discoveryEntry !== undefined) {
            this.addDiscoveryEntryToStore(settings.discoveryEntry);
        } else {
            throw new Error(
                "missing arguments for function CapabilitiesStore.add: neither settings.discoveryEntries nor settings.discoveryEntry are provided"
            );
        }
        return this;
    }

    /**
     * checks if the provided discoveryEntry is not older than the second argument maxAge
     *
     * @param discoveryEntry - the discovery entry to check
     * @param maxAge - the maximum age of the discovery entry
     */
    private checkAge(discoveryEntry: DiscoveryEntry, maxAge?: number): boolean {
        const registrationTime = this.registeredCapabilitiesTime[hashCode(discoveryEntry)];
        if (registrationTime === undefined || maxAge === undefined) {
            return true;
        }
        return Date.now() - registrationTime <= maxAge;
    }

    /**
     * checks if the discoveryQos matches with the discoveryEntry
     *
     * @param discoveryEntry - the discovery entry to check
     * @param cacheMaxAge - the maximum age of the discovery entry
     */
    private qosMatches(discoveryEntry: DiscoveryEntry, cacheMaxAge?: number): boolean {
        return discoveryEntry !== undefined && this.checkAge(discoveryEntry, cacheMaxAge);
    }

    /**
     * filters provided entries and returns only local entries
     *
     * @param entries - the discovery entries to be filtered
     * @param cacheMaxAge - the maximum age of the discovery entries
     */
    private filterEntries(entries: DiscoveryEntry[], cacheMaxAge?: number): DiscoveryEntry[] {
        const returnValue = [];
        for (let i = entries.length - 1; i >= 0; i--) {
            const discoveryEntry = entries[i];
            if (this.qosMatches(discoveryEntry, cacheMaxAge)) {
                returnValue.push(discoveryEntry);
            }
        }
        return returnValue;
    }

    /**
     * Looks up a list of capabilities either for a given participant ID or for a given domain / interfaceName combination
     *
     * @param settings - an object containing the required parameters
     * @param settings.domains - the name of the domains
     * @param settings.interfaceName - the interface name of the capability
     * @param settings.participantId - the participantId of the capability
     * @param settings.cacheMaxAge - max age used for filtering the store entries
     *
     * @returns a list of matching discovery entries.
     */
    public lookup(settings: { domains: string[]; interfaceName: string; cacheMaxAge?: number }): DiscoveryEntry[];
    public lookup(settings: { participantId: string; cacheMaxAge?: number }): DiscoveryEntry[];
    public lookup(settings: any): DiscoveryEntry[] {
        if (settings.domains !== undefined && settings.interfaceName !== undefined) {
            let returnValue: DiscoveryEntry[] = [];
            for (let i = 0; i < settings.domains.length; ++i) {
                const key = getDomainInterfaceNameKey(settings.domains[i], settings.interfaceName);
                const storedEntries = this.discoveryEntryStoreByDomainInterfaceName[key] || [];
                returnValue = returnValue.concat(this.filterEntries(storedEntries, settings.cacheMaxAge));
            }
            return returnValue;
        } else if (settings.participantId !== undefined) {
            const storedEntries = this.discoveryEntryStoreByParticipantId[settings.participantId];
            if (this.qosMatches(storedEntries, settings.cacheMaxAge)) {
                return [storedEntries];
            } else {
                return [];
            }
        } else {
            throw new Error(
                "missing arguments for function CapabilitiesStore.lookup: neither settings.domain + settings.interfaceName nor settings.participantId are provided"
            );
        }
    }

    /**
     * Unregisters either a list of discovery entries or a single discovery entry from the store.
     *
     * @param settings - an object containing the required parameters
     * @param settings.participantIds - an array of participant IDs uniquely identifying the discovery entry to be removed
     * @param settings.participantId -  a participant ID uniquely identifying the discovery entry to be removed
     *
     * @returns returns this for a fluent interface
     */
    public remove(settings: { participantIds?: string[]; participantId?: string }): CapabilitiesStore {
        if (settings.participantIds !== undefined && Typing.getObjectType(settings.participantIds) === "Array") {
            for (let i = 0; i < settings.participantIds.length; i++) {
                this.removeDiscoveryEntryFromStore(settings.participantIds[i]);
            }
        } else if (settings.participantId !== undefined) {
            this.removeDiscoveryEntryFromStore(settings.participantId);
        } else {
            throw new Error(
                "missing arguments for function CapabilitiesStore.remove: neither settings.participantIds nor settings.participantId are provided"
            );
        }

        return this;
    }
}

export = CapabilitiesStore;
