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

define(
        "joynr/capabilities/CapabilitiesStore",
        [ "joynr/util/Typing"
        ],
        function(Typing) {

            /**
             * Private function. This function returns the hashCode of the given discovery entry, this should uniquely identify a
             * discovery entry within the capability directory.
             *
             * @name CapabilitiesStore#hashCode
             * @function
             * @private
             *
             * @param {DiscoveryEntry}
             *            discoveryEntry - discovery entry
             */
            function hashCode(discoveryEntry) {
                return discoveryEntry.domain
                    + discoveryEntry.interfaceName
                    + discoveryEntry.participantId
                    + discoveryEntry.providerVersion
                    + discoveryEntry.publicKeyId;
            }

            /**
             * Private function. Returns a key for indexing by the domain, interfaceName and providerQos.
             *
             * @name CapabilitiesStore#getDomainInterfaceNameKey
             * @function
             * @private
             *
             * @param {String}
             *            domain - the domain
             * @param {String}
             *            interfaceName - the interface name
             * @returns {String} the unique key for the domain and interface name
             */
            function getDomainInterfaceNameKey(domain, interfaceName) {
                return domain + interfaceName;
            }

            /**
             * The Local Capabilities Storage
             *
             * @name CapabilitiesStore
             * @constructor
             *
             * @classdesc
             * The <code>CapabilitiesStore</code> is a joynr internal interface. When a provider that is registered with joynr, the framework creates an
             * entry for that provider in the capabilities directory. These <code>{@link DiscoveryEntry}</code> entries contain access
             * information as well as supported Qos. The information is later used in the arbitration process to pick a provider for a proxy.
             *
             * @param {GlobalDiscoveryEntry[]}
             *            initialCapabilities
             */
            function CapabilitiesStore(initialCapabilities) {
                // participantId -> Array of Capability Information
                var discoveryEntryStoreByParticipantId = {};

                // domain + interface -> Array of GlobalDiscoveryEntry
                var discoveryEntryStoreByDomainInterfaceName = {};

                // discovery entry hashCode -> Array of Capability Information
                var discoveryEntryStore = {};

                var registeredCapabilitiesTime = {};

                /**
                 * Unregisters a discovery entry from the store.
                 *
                 * @name CapabilitiesStore#removeDiscoveryEntryFromStore
                 * @function
                 *
                 * @param {String}
                 *            participantId - the participant ID uniquely identifying the discovery entry to be removed
                 */
                function removeDiscoveryEntryFromStore(participantId) {
                    var domainInterfaceKey, key, capabilities, capId, cap, capFound = false, discoveryEntry;

                    discoveryEntry = discoveryEntryStoreByParticipantId[participantId];

                    // unregister by participant id
                    discoveryEntryStoreByParticipantId[participantId] = undefined;

                    if (discoveryEntry !== undefined) {
                        // unregister by domain and interface
                        domainInterfaceKey =
                                getDomainInterfaceNameKey(
                                        discoveryEntry.domain,
                                        discoveryEntry.interfaceName);
                        capabilities = discoveryEntryStoreByDomainInterfaceName[domainInterfaceKey];

                        capFound = false;
                        for (capId in capabilities) {
                            if (capabilities.hasOwnProperty(capId) && !capFound) {
                                cap = capabilities[capId];
                                if (cap.interfaceName === discoveryEntry.interfaceName
                                    && cap.domain === discoveryEntry.domain) {
                                    capFound = true;
                                    discoveryEntryStoreByDomainInterfaceName[domainInterfaceKey]
                                            .splice(capId, 1);
                                }
                            }
                        }

                        // unregister master store
                        key = hashCode(discoveryEntry);
                        delete discoveryEntryStore[key];
                    }
                }

                /**
                 * Registers discovery entry to the store.
                 *
                 * @name CapabilitiesStore#addDiscoveryEntryToStore
                 * @function
                 *
                 * @param {DiscoveryEntry}
                 *            discoveryEntry - the capability to register
                 *
                 */
                function addDiscoveryEntryToStore(discoveryEntry) {
                    var entryFound = false, isAlreadyThere, discoveryEntries, entryId, entry, domainInterfaceKey, discoveryEntryKey =
                            hashCode(discoveryEntry);

                    // master store, storing key to actual discovery entry object
                    isAlreadyThere = discoveryEntryStore[discoveryEntryKey];
                    if (isAlreadyThere !== undefined) {
                        removeDiscoveryEntryFromStore(isAlreadyThere);
                    }
                    discoveryEntryStore[discoveryEntryKey] = discoveryEntry;
                    registeredCapabilitiesTime[discoveryEntryKey] = Date.now();

                    // by participant id
                    discoveryEntryStoreByParticipantId[discoveryEntry.participantId] =
                            discoveryEntry;

                    // by domain interface and provider qos
                    domainInterfaceKey =
                            getDomainInterfaceNameKey(
                                    discoveryEntry.domain,
                                    discoveryEntry.interfaceName);
                    if (discoveryEntryStoreByDomainInterfaceName[domainInterfaceKey] === undefined) {
                        discoveryEntryStoreByDomainInterfaceName[domainInterfaceKey] = [];
                    }

                    discoveryEntries = discoveryEntryStoreByDomainInterfaceName[domainInterfaceKey];

                    for (entryId in discoveryEntries) {
                        if (discoveryEntries.hasOwnProperty(entryId) && !entryFound) {
                            entry = discoveryEntries[entryId];
                            if (entry.participantId === discoveryEntry.participantId) {
                                entryFound = true;
                            }
                        }
                    }

                    if (!entryFound) {
                        discoveryEntryStoreByDomainInterfaceName[domainInterfaceKey]
                                .push(discoveryEntry);
                    }
                }

                /**
                 * Registers either a single discovery entry or a list of discovery entry to the store.
                 *
                 * @name CapabilitiesStore#add
                 * @function
                 *
                 * @param {Object}
                 *            settings - an object containing the required parameters
                 * @param {Array.<DiscoveryEntry>}
                 *            settings.discoveryEntries - an array of discovery entries to register
                 * @param {DiscoveryEntry}
                 *            settings.discoveryEntry a single discovery entry to register (only if settings.discoveryEntries is not supplied)
                 * @param {Boolean}
                 *            settings.remote true if the provided entry/entries are gathered from remote
                 *
                 * @returns {CapabilitiesStore} returns this for a fluent interface
                 *
                 */
                this.add =
                        function add(settings) {
                            var i;
                            if (settings.discoveryEntries !== undefined
                                && Typing.getObjectType(settings.discoveryEntries) === "Array") {
                                for (i = 0; i < settings.discoveryEntries.length; i++) {
                                    addDiscoveryEntryToStore(settings.discoveryEntries[i]);
                                }
                            } else if (settings.discoveryEntry !== undefined) {
                                addDiscoveryEntryToStore(settings.discoveryEntry);
                            } else {
                                throw new Error(
                                        "missing arguments for function CapabilitiesStore.add: neither settings.discoveryEntries nor settings.discoveryEntry are provided");
                            }
                            return this;
                        };

                /**
                 * checks if the provided discoveryEntry is not older than the second argument maxAge
                 *
                 * @name CapabilitiesStore#checkAge
                 * @function
                 *
                 * @param {DiscoveryEntry} discoveryEntry - the discovery entry to check
                 * @param {Number} maxAge - the maximum age of the discovery entry
                 */
                var checkAge = function checkAge(discoveryEntry, maxAge) {
                    var registrationTime = registeredCapabilitiesTime[hashCode(discoveryEntry)];
                    if (registrationTime === undefined || maxAge === undefined) {
                        return true;
                    }
                    return (Date.now() - registrationTime <= maxAge);
                };

                /**
                 * checks if the discoveryQos matches with the discoveryEntry
                 *
                 * @name CapabilitiesStore#qosMatches
                 * @function
                 *
                 * @param {DiscoveryEntry} discoveryEntry - the discovery entry to check
                 * @param {Number} cacheMaxAge - the maximum age of the discovery entry
                 */
                var qosMatches = function qosMatches(discoveryEntry, cacheMaxAge) {
                    return discoveryEntry !== undefined && checkAge(discoveryEntry, cacheMaxAge);
                };

                /**
                 * filters provided entries and returns only local entries
                 *
                 * @name CapabilitiesStore#filterEntries
                 * @function
                 *
                 * @param {Array.<DiscoveryEntry>} entries - the discovery entries to be filtered
                 * @param {Number} cacheMaxAge - the maximum age of the discovery entries
                 */
                var filterEntries = function filterEntries(entries, cacheMaxAge) {
                    var i, returnValue, discoveryEntry;
                    returnValue = [];
                    for (i = entries.length - 1; i >= 0; i--) {
                        discoveryEntry = entries[i];
                        if (qosMatches(discoveryEntry, cacheMaxAge)) {
                            returnValue.push(discoveryEntry);
                        }
                    }
                    return returnValue;
                };

                /**
                 * Looks up a list of capabilities either for a given participant ID or for a given domain / interfaceName combination
                 *
                 * @name CapabilitiesStore#lookup
                 * @function
                 *
                 * @param {Object}
                 *            settings - an object containing the required parameters
                 * @param {String}
                 *            settings.domains - the name of the domains
                 * @param {String}
                 *            settings.interfaceName - the interface name of the capability
                 * @param {String}
                 *            settings.participantId - the participantId of the capability
                 * @param {DiscoveryQos}
                 *            settings.cacheMaxAge - max age used for filtering the store entries
                 *
                 * @returns {Array} a list of matching discovery entries.
                 */
                this.lookup =
                        function lookup(settings) {
                            var i, key, returnValue = [], storedEntries;
                            if (settings.domains !== undefined
                                && settings.interfaceName !== undefined) {
                                for (i = 0; i < settings.domains.length; ++i) {
                                    key =
                                            getDomainInterfaceNameKey(
                                                    settings.domains[i],
                                                    settings.interfaceName);
                                    storedEntries =
                                            discoveryEntryStoreByDomainInterfaceName[key] || [];
                                    returnValue =
                                            returnValue.concat(filterEntries(
                                                    storedEntries,
                                                    settings.cacheMaxAge));
                                }
                            } else if (settings.participantId !== undefined) {
                                storedEntries =
                                        discoveryEntryStoreByParticipantId[settings.participantId];
                                if (qosMatches(storedEntries, settings.cacheMaxAge)) {
                                    returnValue = [ storedEntries
                                    ];
                                } else {
                                    returnValue = undefined;
                                }
                            } else {
                                throw new Error(
                                        "missing arguments for function CapabilitiesStore.lookup: neither settings.domain + settings.interfaceName nor settings.participantId are provided");
                            }
                            return returnValue;
                        };

                /**
                 * Unregisters either a list of discovery entries or a single discovery entry from the store.
                 *
                 * @name CapabilitiesStore#remove
                 * @function
                 *
                 * @param {Object}
                 *            settings - an object containing the required parameters
                 * @param {Array.<String>}
                 *            settings.participantIds - an array of participant IDs uniquely identifying the discovery entry to be removed
                 * @param {String}
                 *            settings.participantId -  a participant ID uniquely identifying the discovery entry to be removed
                 *
                 * @returns {CapabilitiesStore} returns this for a fluent interface
                 */
                this.remove =
                        function remove(settings) {
                            var i;
                            if (settings.participantIds !== undefined
                                && Typing.getObjectType(settings.participantIds) === "Array") {
                                for (i = 0; i < settings.participantIds.length; i++) {
                                    removeDiscoveryEntryFromStore(settings.participantIds[i]);
                                }
                            } else if (settings.participantId !== undefined) {
                                removeDiscoveryEntryFromStore(settings.participantId);
                            } else {
                                throw new Error(
                                        "missing arguments for function CapabilitiesStore.remove: neither settings.participantIds nor settings.participantId are provided");
                            }

                            return this;
                        };

                if (initialCapabilities && initialCapabilities.length > 0) {
                    this.add({
                        discoveryEntries : initialCapabilities
                    });
                }
            }

            return CapabilitiesStore;

        });
