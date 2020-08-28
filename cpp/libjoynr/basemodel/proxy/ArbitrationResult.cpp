/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
#include "joynr/ArbitrationResult.h"

namespace joynr
{

ArbitrationResult::ArbitrationResult(
        const std::vector<types::DiscoveryEntryWithMetaInfo>& discoveryEntries)
        : _discoveryEntries(discoveryEntries)
{
}

const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& ArbitrationResult::
        getDiscoveryEntries() const
{
    return _discoveryEntries;
}

void ArbitrationResult::setDiscoveryEntries(
        const std::vector<types::DiscoveryEntryWithMetaInfo>& discoveryEntries)
{
    this->_discoveryEntries = discoveryEntries;
}

bool ArbitrationResult::isEmpty() const
{
    return _discoveryEntries.empty();
}

} // namespace joynr
