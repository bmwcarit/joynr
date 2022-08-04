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
/**
 * @param subscriptions - Map containing the subscriptions to be serialized
 *
 * @returns serialized subscriptions
 */
export function serializeSubscriptions(subscriptions: Record<string, any>): string {
    const result = [];
    for (const subscriptionId in subscriptions) {
        if (Object.prototype.hasOwnProperty.call(subscriptions, subscriptionId)) {
            result[result.length] = subscriptions[subscriptionId];
        }
    }

    return JSON.stringify(result);
}

/**
 * @param subscriptions - Map containing the subscriptions to be serialized
 *
 * @returns serialized subscriptionIds
 */
export function serializeSubscriptionIds(subscriptions: Record<string, any>): string {
    const result = [];

    for (const subscriptionId in subscriptions) {
        if (Object.prototype.hasOwnProperty.call(subscriptions, subscriptionId)) {
            result[result.length] = subscriptionId;
        }
    }

    return JSON.stringify(result);
}

/**
 * @param subscriptions - serialized subscriptions as String
 *
 * @returns deserialized subscriptions as Map
 */
export function deserializeSubscriptions(subscriptions: string): Record<string, any> {
    let array;
    const result: Record<string, any> = {};
    if (JSON && JSON.parse) {
        try {
            array = JSON.parse(subscriptions);
        } catch (err) {
            throw new Error(err);
        }
    }

    for (const subscription in array) {
        if (Object.prototype.hasOwnProperty.call(array, subscription)) {
            const object = array[subscription];
            result[object.subscriptionId] = object;
        }
    }
    return result;
}

/**
 * @param subscriptions - serialized subscriptions as String
 *
 * @returns deserialized subscriptionIds as Array of String
 */
export function deserializeSubscriptionIds(subscriptions: string): string[] {
    return JSON.parse(subscriptions);
}

/**
 * @param expectedFilterParameters - the expected filter parameters of a broadcast subscription
 * @param actualFilterParameters - the actual filter parameters of a broadcast subscription
 * @param broadcastName - the name of the checked broadcast
 *
 * @returns an object containing possible caughtErrors if the actualFilterParameters do not match
 *                   the expected filter parameters
 */
export function checkFilterParameters(
    expectedFilterParameters: Record<string, any>,
    actualFilterParameters: Record<string, any>,
    broadcastName: string
): { caughtErrors: string[] } {
    const result: { caughtErrors: string[] } = {
        caughtErrors: []
    };
    if (
        actualFilterParameters === undefined ||
        actualFilterParameters === null ||
        Object.keys(actualFilterParameters).length === 0
    ) {
        return result;
    }
    const targetKeys = Object.keys(expectedFilterParameters);
    const sourceKeys = Object.keys(actualFilterParameters);
    for (let i = 0; i < targetKeys.length; i++) {
        if (sourceKeys.indexOf(targetKeys[i]) === -1) {
            result.caughtErrors.push(
                `Filter parameter ${targetKeys[i]} for broadcast "${broadcastName}" is not provided`
            );
        }
    }
    return result;
}

/**
 * @param providerParticipantId - provider's participant ID
 * @param multicastName - the name of the multicasts
 * @param partitions - partitions of this multicast
 */
export function createMulticastId(providerParticipantId: string, multicastName: string, partitions?: any[]): string {
    let multicastId = `${providerParticipantId}/${multicastName}`;
    if (partitions !== undefined) {
        for (let i = 0; i < partitions.length; i++) {
            multicastId += `/${partitions[i]}`;
        }
    }
    return multicastId;
}

export const VALID_PARTITION_REGEX = /^[a-zA-Z0-9]+$/;
export const SINGLE_POSITION_WILDCARD = "+";
export const MULTI_LEVEL_WILDCARD = "*";

/**
 * validates if the provided partitions for multicast publications only contains valid characters
 * @param partitions
 * @throws {Error} if partitions contains invalid characters
 */
export function validatePartitions(partitions?: string[]): void {
    if (partitions !== undefined) {
        for (let i = 0; i < partitions.length; i++) {
            const partition = partitions[i];
            if (
                !partition.match(VALID_PARTITION_REGEX) &&
                !(partition === SINGLE_POSITION_WILDCARD) &&
                !(i + 1 === partitions.length && partition === MULTI_LEVEL_WILDCARD)
            ) {
                throw new Error(
                    `Partition ${partitions[i]} contains invalid characters.%n` +
                        `Must only contain a-z A-Z 0-9, or by a single position wildcard (+),%n` +
                        `or the last partition may be a multi-level wildcard (*).`
                );
            }
        }
    }
}
