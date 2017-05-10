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

#include "joynr/CapabilityUtils.h"

#include <algorithm>

#include "joynr/types/DiscoveryEntry.h"
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"

namespace joynr
{
namespace util
{

joynr::types::DiscoveryEntryWithMetaInfo convert(bool isLocal,
                                                 const joynr::types::DiscoveryEntry& entry)
{
    return joynr::types::DiscoveryEntryWithMetaInfo(entry.getProviderVersion(),
                                                    entry.getDomain(),
                                                    entry.getInterfaceName(),
                                                    entry.getParticipantId(),
                                                    entry.getQos(),
                                                    entry.getLastSeenDateMs(),
                                                    entry.getExpiryDateMs(),
                                                    entry.getPublicKeyId(),
                                                    isLocal);
}

std::vector<types::DiscoveryEntryWithMetaInfo> convert(
        bool isLocal,
        const std::vector<types::DiscoveryEntry>& entries)
{
    std::vector<types::DiscoveryEntryWithMetaInfo> result;
    result.reserve(entries.size());

    std::transform(
            entries.begin(),
            entries.end(),
            std::back_inserter(result),
            [isLocal](const types::DiscoveryEntry& entry) { return convert(isLocal, entry); });

    return result;
}

} // namespace util
} // namespace joynr
