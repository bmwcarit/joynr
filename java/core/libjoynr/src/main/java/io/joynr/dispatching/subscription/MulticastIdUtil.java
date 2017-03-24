package io.joynr.dispatching.subscription;

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

import java.util.regex.Pattern;

import io.joynr.exceptions.JoynrIllegalStateException;

public final class MulticastIdUtil {

    private static final Pattern VALID_PARTITION_REGEX = Pattern.compile("^[a-zA-Z0-9]+$");

    private static final String SINGLE_POSITION_WILDCARD = "+";

    private static final String MULTI_LEVEL_WILDCARD = "*";

    private MulticastIdUtil() {
    }

    public static String createMulticastId(String providerParticipantId, String multicastName, String... partitions) {
        StringBuilder builder = new StringBuilder(providerParticipantId);
        builder.append("/").append(multicastName);
        if (partitions != null && partitions.length > 0) {
            for (int index = 0; index < partitions.length; index++) {
                builder.append("/").append(validatePartition(partitions, index));
            }
        }
        return builder.toString();
    }

    private static String validatePartition(String[] partitions, int index) {
        String partition = partitions[index];
        if (!VALID_PARTITION_REGEX.matcher(partition).matches() && !SINGLE_POSITION_WILDCARD.equals(partition)
                && !(partitions.length == index + 1 && MULTI_LEVEL_WILDCARD.equals(partition))) {
            throw new JoynrIllegalStateException(String.format("Partition %s contains invalid characters.%n"
                    + "Must only contain a-z A-Z 0-9, or by a single position wildcard (+),%n"
                    + "or the last partition may be a multi-level wildcard (*).", partition));
        }
        return partition;
    }

    public static String sanitizeForPartition(String value) {
        return value.replaceAll("[^a-zA-Z0-9]", "");
    }
}
