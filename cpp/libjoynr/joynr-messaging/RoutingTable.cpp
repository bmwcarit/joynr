/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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

#include "joynr/RoutingTable.h"

namespace joynr
{
INIT_LOGGER(RoutingTable);

RoutingTable::RoutingTable() : multiIndexContainer()
{
}

RoutingTable::~RoutingTable()
{
    JOYNR_LOG_TRACE(logger, "destructor: number of entries = {}", multiIndexContainer.size());
}

boost::optional<routingtable::RoutingEntry> RoutingTable::lookupRoutingEntryByParticipantId(
        const std::string& participantId) const
{
    auto& index = boost::multi_index::get<routingtable::tags::ParticipantId>(multiIndexContainer);
    auto found = index.find(participantId);
    if (found == index.end()) {
        return boost::none;
    }
    return *found;
}

std::unordered_set<std::string> RoutingTable::lookupParticipantIdsByAddress(
        std::shared_ptr<const joynr::system::RoutingTypes::Address> searchValue) const
{
    std::unordered_set<std::string> result;
    const auto& addressIndex =
            boost::multi_index::get<routingtable::tags::Address>(multiIndexContainer);
    auto found = addressIndex.equal_range(searchValue);
    for (auto it = found.first; it != found.second; ++it) {
        result.insert(it->participantId);
    }
    return result;
}

bool RoutingTable::containsParticipantId(const std::string& participantId) const
{
    auto& index = boost::multi_index::get<routingtable::tags::ParticipantId>(multiIndexContainer);
    auto found = index.find(participantId);
    return found != index.end();
}

void RoutingTable::add(const std::string& participantId,
                       bool isGloballyVisible,
                       std::shared_ptr<const joynr::system::RoutingTypes::Address> address)
{
    routingtable::RoutingEntry routingEntry(participantId, std::move(address), isGloballyVisible);
    auto result = multiIndexContainer.insert(routingEntry);
    if (!result.second) {
        multiIndexContainer.replace(result.first, routingEntry);
    }
    JOYNR_LOG_TRACE(logger, "Added participantId: {}", participantId);
}

void RoutingTable::remove(const std::string& participantId)
{
    JOYNR_LOG_TRACE(logger, "Removing registered participantId: {}", participantId);
    multiIndexContainer.erase(participantId);
}

bool RoutingTable::AddressEqual::operator()(
        std::shared_ptr<const joynr::system::RoutingTypes::Address> lhs,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> rhs) const
{
    bool result = ((*lhs) == (*rhs));
    return result;
}

size_t RoutingTable::AddressHash::operator()(
        std::shared_ptr<const joynr::system::RoutingTypes::Address> address) const
{
    return address->hashCode();
}

} // namespace joynr
