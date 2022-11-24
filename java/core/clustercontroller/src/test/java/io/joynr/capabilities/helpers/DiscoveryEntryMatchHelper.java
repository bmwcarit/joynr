/*
 * #%L
 * %%
 * Copyright (C) 2022 BMW Car IT GmbH
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
package io.joynr.capabilities.helpers;

import java.util.Objects;

import joynr.types.DiscoveryEntry;
import joynr.types.DiscoveryEntryWithMetaInfo;
import joynr.types.GlobalDiscoveryEntry;

public class DiscoveryEntryMatchHelper {

    public static boolean discoveryEntriesMatchWithUpdatedLastSeenDate(final DiscoveryEntry expected,
                                                                       final DiscoveryEntry actual) {
        return Objects.equals(expected.getDomain(), actual.getDomain())
                && Objects.equals(expected.getExpiryDateMs(), actual.getExpiryDateMs())
                && Objects.equals(expected.getInterfaceName(), actual.getInterfaceName())
                && Objects.equals(expected.getParticipantId(), actual.getParticipantId())
                && expected.getProviderVersion().equals(actual.getProviderVersion())
                && Objects.equals(expected.getPublicKeyId(), actual.getPublicKeyId())
                && expected.getQos().equals(actual.getQos())
                && expected.getLastSeenDateMs() <= actual.getLastSeenDateMs()
                && (expected.getLastSeenDateMs() + 1000) >= actual.getLastSeenDateMs();
    }

    public static boolean globalDiscoveryEntriesMatchWithUpdatedLastSeenDate(final GlobalDiscoveryEntry expected,
                                                                             final GlobalDiscoveryEntry actual) {
        return discoveryEntriesMatchWithUpdatedLastSeenDate(expected, actual)
                && expected.getAddress().equals(actual.getAddress());
    }

    public static boolean discoveryEntriesWithMetaInfoMatchWithUpdatedLastSeenDate(final DiscoveryEntryWithMetaInfo expected,
                                                                                   final DiscoveryEntryWithMetaInfo actual) {
        return discoveryEntriesMatchWithUpdatedLastSeenDate(expected, actual)
                && expected.getIsLocal() == actual.getIsLocal();
    }
}
