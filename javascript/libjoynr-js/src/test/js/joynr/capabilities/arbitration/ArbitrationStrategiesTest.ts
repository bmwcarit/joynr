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

import * as ArbitrationStrategyCollection from "../../../../../main/js/joynr/types/ArbitrationStrategyCollection";
import DiscoveryEntryWithMetaInfo from "../../../../../main/js/generated/joynr/types/DiscoveryEntryWithMetaInfo";
import ProviderScope from "../../../../../main/js/generated/joynr/types/ProviderScope";
import ProviderQos from "../../../../../main/js/generated/joynr/types/ProviderQos";
import CustomParameter from "../../../../../main/js/generated/joynr/types/CustomParameter";
import Version from "../../../../../main/js/generated/joynr/types/Version";

const expiryDateMs = Date.now() + 1e10;

describe("libjoynr-js.joynr.types.ArbitrationStrategyCollection", () => {
    it("is defined and of correct type", () => {
        expect(ArbitrationStrategyCollection).toBeDefined();
        expect(ArbitrationStrategyCollection).not.toBeNull();
        expect(typeof ArbitrationStrategyCollection === "object").toBeTruthy();
    });

    it("has all required strategies of type function", () => {
        expect(ArbitrationStrategyCollection.Nothing).toBeDefined();
        expect(ArbitrationStrategyCollection.HighestPriority).toBeDefined();
        expect(ArbitrationStrategyCollection.Keyword).toBeDefined();
        expect(ArbitrationStrategyCollection.LastSeen).toBeDefined();
        expect(ArbitrationStrategyCollection.FixedParticipant).toBeDefined();

        expect(typeof ArbitrationStrategyCollection.Nothing).toBe("function");
        expect(typeof ArbitrationStrategyCollection.HighestPriority).toBe("function");
        expect(typeof ArbitrationStrategyCollection.Keyword).toBe("function");
        expect(typeof ArbitrationStrategyCollection.LastSeen).toBe("function");
        expect(typeof ArbitrationStrategyCollection.FixedParticipant).toBe("function");
    });

    function getDiscoveryEntryWithMetaInfoForFixedParticipantId() {
        return [
            new DiscoveryEntryWithMetaInfo({
                providerVersion: new Version({
                    majorVersion: 47,
                    minorVersion: 11
                }),
                domain: "myDomain",
                interfaceName: "myInterfaceName",
                lastSeenDateMs: 111,
                qos: new ProviderQos({
                    customParameters: [],
                    priority: 1,
                    scope: ProviderScope.GLOBAL,
                    supportsOnChangeSubscriptions: true
                }),
                participantId: "myFixedParticipantId",
                isLocal: false,
                publicKeyId: "",
                expiryDateMs
            })
        ];
    }

    function getDiscoveryEntryWithMetaInfoList() {
        return [
            new DiscoveryEntryWithMetaInfo({
                providerVersion: new Version({
                    majorVersion: 47,
                    minorVersion: 11
                }),
                domain: "KeywordmyDomain",
                interfaceName: "myInterfaceName",
                lastSeenDateMs: 111,
                qos: new ProviderQos({
                    customParameters: [],
                    priority: 1,
                    scope: ProviderScope.GLOBAL,
                    supportsOnChangeSubscriptions: true
                }),
                participantId: "1",
                isLocal: false,
                publicKeyId: "",
                expiryDateMs
            }),
            new DiscoveryEntryWithMetaInfo({
                providerVersion: new Version({
                    majorVersion: 47,
                    minorVersion: 11
                }),
                domain: "myDomain",
                interfaceName: "myInterfaceName",
                lastSeenDateMs: 333,
                qos: new ProviderQos({
                    customParameters: [],
                    priority: 4,
                    scope: ProviderScope.GLOBAL,
                    supportsOnChangeSubscriptions: true
                }),
                participantId: "1",
                isLocal: false,
                publicKeyId: "",
                expiryDateMs
            }),
            new DiscoveryEntryWithMetaInfo({
                providerVersion: new Version({
                    majorVersion: 47,
                    minorVersion: 11
                }),
                domain: "myWithKeywordDomain",
                interfaceName: "myInterfaceName",
                lastSeenDateMs: 222,
                qos: new ProviderQos({
                    customParameters: [
                        new CustomParameter({
                            name: "keyword",
                            value: "myKeyword"
                        }),
                        new CustomParameter({
                            name: "thename",
                            value: "theValue"
                        })
                    ],
                    priority: 3,
                    scope: ProviderScope.GLOBAL,
                    supportsOnChangeSubscriptions: true
                }),
                participantId: "1",
                isLocal: false,
                publicKeyId: "",
                expiryDateMs
            }),
            new DiscoveryEntryWithMetaInfo({
                providerVersion: new Version({
                    majorVersion: 47,
                    minorVersion: 11
                }),
                domain: "myDomain",
                interfaceName: "myInterfaceNameKeyword",
                lastSeenDateMs: 555,
                qos: new ProviderQos({
                    customParameters: [
                        new CustomParameter({
                            name: "keyword",
                            value: "wrongKeyword"
                        }),
                        new CustomParameter({
                            name: "theName",
                            value: "theValue"
                        })
                    ],
                    priority: 5,
                    scope: ProviderScope.GLOBAL,
                    supportsOnChangeSubscriptions: true
                }),
                participantId: "1",
                isLocal: false,
                publicKeyId: "",
                expiryDateMs
            }),
            new DiscoveryEntryWithMetaInfo({
                providerVersion: new Version({
                    majorVersion: 47,
                    minorVersion: 11
                }),
                domain: "myDomain",
                interfaceName: "myInterfaceName",
                lastSeenDateMs: 444,
                qos: new ProviderQos({
                    customParameters: [
                        new CustomParameter({
                            name: "keyword",
                            value: "myKeyword"
                        }),
                        new CustomParameter({
                            name: "theName",
                            value: "theValue"
                        })
                    ],
                    priority: 2,
                    scope: ProviderScope.GLOBAL,
                    supportsOnChangeSubscriptions: true
                }),
                participantId: "1",
                isLocal: false,
                publicKeyId: "",
                expiryDateMs
            })
        ];
    }

    it("Strategy 'Nothing' does nothing", () => {
        expect(ArbitrationStrategyCollection.Nothing(getDiscoveryEntryWithMetaInfoList())).toEqual(
            getDiscoveryEntryWithMetaInfoList()
        );
    });

    it("Strategy 'HighestPriority' includes all capability infos", () => {
        const discoveryEntryList = getDiscoveryEntryWithMetaInfoList();
        let discoveryEntryId: any;
        const highestPriority = ArbitrationStrategyCollection.HighestPriority(discoveryEntryList);
        for (discoveryEntryId in discoveryEntryList) {
            if (discoveryEntryList.hasOwnProperty(discoveryEntryId)) {
                expect(highestPriority).toContain(discoveryEntryList[discoveryEntryId]);
            }
        }
    });

    it("Strategy 'HighestPriority' sorts according to providerQos priority", () => {
        const highestPriority = ArbitrationStrategyCollection.HighestPriority(getDiscoveryEntryWithMetaInfoList());
        let i: any;
        for (i = 0; i < highestPriority.length - 1; ++i) {
            expect(highestPriority[i].qos.priority).toBeGreaterThan(highestPriority[i + 1].qos.priority);
        }
    });

    it("Strategy 'LastSeen' includes all capability infos", () => {
        const discoveryEntryList = getDiscoveryEntryWithMetaInfoList();
        let discoveryEntryId: any;
        const latestSeen = ArbitrationStrategyCollection.LastSeen(discoveryEntryList);
        for (discoveryEntryId in discoveryEntryList) {
            if (discoveryEntryList.hasOwnProperty(discoveryEntryId)) {
                expect(latestSeen).toContain(discoveryEntryList[discoveryEntryId]);
            }
        }
    });

    it("Strategy 'LastSeen' sorts according to lastSeenDateMs priority", () => {
        const lastSeen = ArbitrationStrategyCollection.LastSeen(getDiscoveryEntryWithMetaInfoList());
        let i: any;
        for (i = 0; i < lastSeen.length - 1; ++i) {
            expect(lastSeen[i].lastSeenDateMs).toBeGreaterThan(lastSeen[i + 1].lastSeenDateMs);
        }
    });

    it("Strategy 'FixedParticipantId' gets discovered capability based on fixed participantId", () => {
        const discoveredEntries = ArbitrationStrategyCollection.FixedParticipant(
            getDiscoveryEntryWithMetaInfoForFixedParticipantId()
        );
        expect(discoveredEntries.length).toBe(1);
        expect(discoveredEntries[0].participantId).toBe("myFixedParticipantId");
        expect(discoveredEntries[0].interfaceName).toBe("myInterfaceName");
    });

    it("Strategy 'Keyword' only includes capability infos that have the keyword Qos set to 'myKeyword'", () => {
        let discoveryEntryId: any;
        let qosId: any;
        let found: any;
        let qosParam: any;
        const keyword = "myKeyword";
        const discoveryEntryList = getDiscoveryEntryWithMetaInfoList();
        // The arbitrator only calls the strategy with the list
        // of capabillities, so Keyword should be tested with bind()
        // which however is not supported by PhantomJS 1
        /*
                        var arbitrationStrategy =
                            ArbitrationStrategyCollection.Keyword.bind(
                                    undefined,
                                    keyword);
                        var keywordCapInfoList =
                            arbitrationStrategy(discoveryEntryList);
                         */
        const keywordCapInfoList = ArbitrationStrategyCollection.Keyword(keyword, discoveryEntryList);
        expect(keywordCapInfoList.length).toBe(2);
        for (discoveryEntryId in discoveryEntryList) {
            if (discoveryEntryList.hasOwnProperty(discoveryEntryId)) {
                const capInfo = discoveryEntryList[discoveryEntryId];
                found = false;
                if (capInfo.qos.customParameters && Array.isArray(capInfo.qos.customParameters)) {
                    for (qosId in capInfo.qos.customParameters) {
                        if (capInfo.qos.customParameters.hasOwnProperty(qosId)) {
                            qosParam = capInfo.qos.customParameters[qosId];
                            if (!found && qosParam && qosParam.value && qosParam.value === keyword) {
                                found = true;
                            }
                        }
                    }
                }
                if (found) {
                    expect(keywordCapInfoList).toContain(capInfo);
                } else {
                    expect(keywordCapInfoList).not.toContain(capInfo);
                }
            }
        }
    });

    it("Strategy 'Keyword' only matches against CustomParameters with name 'keyword' and the right keyword", () => {
        const rightKeyword = "rightKeyword";
        const rightName = "keyword";
        const wrongName = "wrongName";
        const wrongKeyword = "wrongKeyword";

        // this object isn't a practical input. For the tests unnecessary keys got removed
        const discoveryEntryList = [
            {
                domain: "correct",
                qos: {
                    customParameters: [
                        new CustomParameter({
                            name: rightName,
                            value: rightKeyword
                        })
                    ]
                }
            },
            {
                domain: "wrongName",
                qos: {
                    customParameters: [
                        new CustomParameter({
                            name: wrongName,
                            value: wrongKeyword
                        })
                    ]
                }
            },
            {
                domain: "wrongKeyword",
                qos: {
                    customParameters: [
                        new CustomParameter({
                            name: rightName,
                            value: wrongKeyword
                        })
                    ]
                }
            },
            {
                domain: "allWrong",
                qos: {
                    customParameters: [
                        new CustomParameter({
                            name: wrongName,
                            value: wrongKeyword
                        })
                    ]
                }
            }
        ];

        const keywordCapInfoList = ArbitrationStrategyCollection.Keyword(rightKeyword, discoveryEntryList as any);
        expect(keywordCapInfoList.length).toBe(1);
        expect(keywordCapInfoList[0].domain).toEqual("correct");
    });
});
