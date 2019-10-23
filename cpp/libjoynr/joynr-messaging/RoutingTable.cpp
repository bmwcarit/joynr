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
#include "joynr/system/RoutingTypes/MqttAddress.h"

namespace joynr
{

RoutingTable::RoutingTable(const std::string& gcdParticipantId)
        : _multiIndexContainer(), _gcdParticipantId(gcdParticipantId)
{
}

RoutingTable::~RoutingTable()
{
    JOYNR_LOG_TRACE(logger(), "destructor: number of entries = {}", _multiIndexContainer.size());
}

boost::optional<routingtable::RoutingEntry> RoutingTable::lookupRoutingEntryByParticipantId(
        const std::string& participantId) const
{
    auto& index = boost::multi_index::get<routingtable::tags::ParticipantId>(_multiIndexContainer);
    auto found = index.find(participantId);
    if (found == index.end()) {
        return boost::none;
    }
    return *found;
}

boost::optional<routingtable::RoutingEntry> RoutingTable::lookupRoutingEntryByParticipantIdAndGbid(
        const std::string& participantId,
        const std::string& gbid) const
{
    auto found = lookupRoutingEntryByParticipantId(participantId);
    if (found && (participantId == this->_gcdParticipantId)) {
        auto address = found->address;
        if (auto mqttAddress =
                    dynamic_cast<const joynr::system::RoutingTypes::MqttAddress*>(address.get())) {
            const auto newMqttAddress = std::make_shared<joynr::system::RoutingTypes::MqttAddress>(
                    gbid, mqttAddress->getTopic());
            return routingtable::RoutingEntry(participantId,
                                              newMqttAddress,
                                              found->isGloballyVisible,
                                              found->_expiryDateMs,
                                              found->_isSticky);
        }
    }
    return found;
}

std::unordered_set<std::string> RoutingTable::lookupParticipantIdsByAddress(
        std::shared_ptr<const joynr::system::RoutingTypes::Address> searchValue) const
{
    std::unordered_set<std::string> result;
    const auto& addressIndex =
            boost::multi_index::get<routingtable::tags::Address>(_multiIndexContainer);
    auto found = addressIndex.equal_range(searchValue);
    for (auto it = found.first; it != found.second; ++it) {
        result.insert(it->participantId);
    }
    return result;
}

bool RoutingTable::containsParticipantId(const std::string& participantId) const
{
    auto& index = boost::multi_index::get<routingtable::tags::ParticipantId>(_multiIndexContainer);
    auto found = index.find(participantId);
    return found != index.end();
}

void RoutingTable::add(const std::string& participantId,
                       bool isGloballyVisible,
                       std::shared_ptr<const joynr::system::RoutingTypes::Address> address,
                       std::int64_t expiryDateMs,
                       bool isSticky)
{
    routingtable::RoutingEntry routingEntry(participantId,
                                            std::move(address),
                                            std::move(isGloballyVisible),
                                            std::move(expiryDateMs),
                                            std::move(isSticky));
    auto result = _multiIndexContainer.insert(routingEntry);
    if (!result.second) {
        _multiIndexContainer.replace(result.first, routingEntry);
        JOYNR_LOG_INFO(logger(),
                       "Replaced routing entry: new: {}, old: {}, #entries: {}",
                       routingEntry.toString(),
                       result.first->toString(),
                       _multiIndexContainer.size());
    } else {
        JOYNR_LOG_INFO(logger(),
                       "Added routing entry: {}, #entries: {}",
                       routingEntry.toString(),
                       _multiIndexContainer.size());
    }
}

void RoutingTable::remove(const std::string& participantId)
{
    const auto routingEntry = lookupRoutingEntryByParticipantId(participantId);
    if (routingEntry && routingEntry->_isSticky) {
        JOYNR_LOG_WARN(logger(),
                       "Cannot remove sticky routing entry (participantId={}, address={}, "
                       "isGloballyVisible={}, expiryDateMs={}) from routing table",
                       participantId,
                       routingEntry->address->toString(),
                       routingEntry->isGloballyVisible,
                       routingEntry->_expiryDateMs);
        return;
    }
    JOYNR_LOG_INFO(logger(),
                   "Removing routing entry for participantId: {}, #entries before removal: {}",
                   participantId,
                   _multiIndexContainer.size());
    _multiIndexContainer.erase(participantId);
}

void RoutingTable::purge()
{
    bool expiredEntriesFound = false;
    auto& index = boost::multi_index::get<routingtable::tags::ExpiryDate>(_multiIndexContainer);
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::system_clock::now().time_since_epoch()).count();
    auto last = index.upper_bound(now);
    std::vector<std::string> expiredParticipantIds;
    for (auto routingEntryIterator = index.lower_bound(0); routingEntryIterator != last;
         ++routingEntryIterator) {
        if (!routingEntryIterator->_isSticky) {
            expiredParticipantIds.push_back(routingEntryIterator->participantId);
            expiredEntriesFound = true;
        }
    }
    if (expiredEntriesFound) {
        JOYNR_LOG_INFO(logger(), "Purging expired routing entries");
    }
    for (auto& participantId : expiredParticipantIds) {
        remove(participantId);
    }
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
