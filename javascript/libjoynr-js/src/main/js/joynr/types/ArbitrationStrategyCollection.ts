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
import * as DiscoveryEntryWithMetaInfo from "../../generated/joynr/types/DiscoveryEntryWithMetaInfo";
import { KEYWORD_PARAMETER } from "./ArbitrationConstants";

/**
 * The ArbitrationStrategyCollection.Nothing just returns the passed array
 *
 * @param capabilities the array of capability entries
 * @param capabilities.array the array of DiscoveryEntries
 *
 * @returns an array of capabilities as it came in through the
 *          capabilities parameter
 */
export function Nothing(capabilities: DiscoveryEntryWithMetaInfo[]): DiscoveryEntryWithMetaInfo[] {
    return capabilities;
}

/**
 * The ArbitrationStrategyCollection.HighestPriority favors higher priorities
 *
 * @param capabilities the array of capability entries
 *
 * @returns an array of capabilities sorted by the highest priority
 */
export function HighestPriority(capabilities: DiscoveryEntryWithMetaInfo[]): DiscoveryEntryWithMetaInfo[] {
    if (!Array.isArray(capabilities)) {
        throw new Error("provided argument capabilities is not of type Array");
    }

    // sort with descending priority
    return capabilities.sort((a, b) => {
        return b.qos.priority - a.qos.priority;
    });
}

/**
 * The ArbitrationStrategyCollection.Keyword searches for the given keyword in the
 * "keyword" qos parameter and returns those.
 * NOTE: to use this function, you must bind your keyword as follows:
 * arbitrationStrategy = Keyword.bind(undefined, "myKeyword")
 *
 * @param capabilities the array of capability entries
 * @param keyword the keyword to search for
 * @returns an array of capabilities sorted by the highest priority
 */
export function Keyword(keyword: string, capabilities: DiscoveryEntryWithMetaInfo[]): DiscoveryEntryWithMetaInfo[] {
    const keywordCaps = [];

    if (!Array.isArray(capabilities)) {
        throw new Error("provided argument capabilities is not of type Array");
    }

    for (const capId in capabilities) {
        if (capabilities.hasOwnProperty(capId)) {
            const cap = capabilities[capId];
            if (cap.qos.customParameters && Array.isArray(cap.qos.customParameters)) {
                for (const qosId in cap.qos.customParameters) {
                    if (cap.qos.customParameters.hasOwnProperty(qosId)) {
                        const qosParam = cap.qos.customParameters[qosId];
                        if (
                            qosParam &&
                            qosParam.value &&
                            qosParam.name === KEYWORD_PARAMETER &&
                            qosParam.value === keyword
                        ) {
                            keywordCaps.push(cap);
                        }
                    }
                }
            }
        }
    }
    return keywordCaps;
}

/**
 * The ArbitrationStrategyCollection.LastSeen favors latest lastSeenDateMs
 *
 * @param capabilities the array of capability entries
 * @param capabilities.array the array of capability entries
 *
 * @returns an array of capabilities sorted by the lastSeenDateMs
 */
export function LastSeen(capabilities: DiscoveryEntryWithMetaInfo[]): DiscoveryEntryWithMetaInfo[] {
    if (!Array.isArray(capabilities)) {
        throw new Error("provided argument capabilities is not of type Array");
    }

    // sort with descending priority
    return capabilities.sort((a, b) => {
        return b.lastSeenDateMs - a.lastSeenDateMs;
    });
}
/**
 * The ArbitrationStrategyCollection.FixedParticipant returns an array of one entry with the FixedParticipantId
 *
 * @param capabilities the array of capability entries
 * @param capabilities.array the array of capability entries
 *
 * @returns an array of one and solely capability which is the one of FixedParticipantId
 */
export function FixedParticipant(capabilities: DiscoveryEntryWithMetaInfo[]): DiscoveryEntryWithMetaInfo[] {
    if (!Array.isArray(capabilities)) {
        throw new Error("provided argument capabilities is not of type Array");
    }

    if (capabilities.length > 1) {
        throw new Error("multiple providers found for the given fixed participantId");
    }
    return capabilities;
}
