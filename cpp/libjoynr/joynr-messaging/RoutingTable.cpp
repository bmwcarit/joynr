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

RoutingTable::RoutingTable(std::vector<std::string> gbidVector) : multiIndexContainer()
{
    this->gbidVector = gbidVector;
}

RoutingTable::~RoutingTable()
{
    JOYNR_LOG_TRACE(logger(), "destructor: number of entries = {}", multiIndexContainer.size());
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

boost::optional<routingtable::RoutingEntry> RoutingTable::lookupRoutingEntryByParticipantIdAndGbid(
        const std::string& participantId,
        const std::string& gbid) const
{
    auto found = lookupRoutingEntryByParticipantId(participantId);
    if (found && participantId == this->gcdParticipantId) {
        if (std::find(gbidVector.begin(), gbidVector.end(), gbid) != gbidVector.end()) {
            auto address = found->address;
            if (dynamic_cast<const joynr::system::RoutingTypes::MqttAddress*>(address.get()) !=
                nullptr) {
                auto mqttAddress = dynamic_cast<const joynr::system::RoutingTypes::MqttAddress*>(
                        address.get());
                const auto newMqttAddress =
                        std::make_shared<joynr::system::RoutingTypes::MqttAddress>(
                                joynr::system::RoutingTypes::MqttAddress(
                                        gbid, mqttAddress->getTopic()));
                const auto newRoutingEntry = routingtable::RoutingEntry(participantId,
                                                                        newMqttAddress,
                                                                        found->isGloballyVisible,
                                                                        found->expiryDateMs,
                                                                        found->isSticky);
                return newRoutingEntry;
            }
        } else {
            JOYNR_LOG_ERROR(logger(),
                            "The provided gbid {} for the participantId {} is unknown!",
                            gbid,
                            participantId);
            return boost::none;
        }
    } else {
        return found;
    }
}

void RoutingTable::setGcdParticipantId(std::string participantId)
{
    gcdParticipantId = participantId;
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
                       std::shared_ptr<const joynr::system::RoutingTypes::Address> address,
                       std::int64_t expiryDateMs,
                       bool isSticky)
{
    routingtable::RoutingEntry routingEntry(participantId,
                                            std::move(address),
                                            std::move(isGloballyVisible),
                                            std::move(expiryDateMs),
                                            std::move(isSticky));
    auto result = multiIndexContainer.insert(routingEntry);
    if (!result.second) {
        multiIndexContainer.replace(result.first, routingEntry);
        JOYNR_LOG_INFO(logger(),
                       "Replaced routing entry: new: {}, old: {}, #entries: {}",
                       routingEntry.toString(),
                       result.first->toString(),
                       multiIndexContainer.size());
    } else {
        JOYNR_LOG_INFO(logger(),
                       "Added routing entry: {}, #entries: {}",
                       routingEntry.toString(),
                       multiIndexContainer.size());
    }
}

void RoutingTable::remove(const std::string& participantId)
{
    const auto routingEntry = lookupRoutingEntryByParticipantId(participantId);
    if (routingEntry && routingEntry->isSticky) {
        JOYNR_LOG_WARN(logger(),
                       "Cannot remove sticky routing entry (participantId={}, address={}, "
                       "isGloballyVisible={}, expiryDateMs={}) from routing table",
                       participantId,
                       routingEntry->address->toString(),
                       routingEntry->isGloballyVisible,
                       routingEntry->expiryDateMs);
        return;
    }
    JOYNR_LOG_INFO(logger(),
                   "Removing routing entry for participantId: {}, #entries before removal: {}",
                   participantId,
                   multiIndexContainer.size());
    multiIndexContainer.erase(participantId);
}

void RoutingTable::purge()
{
    bool expiredEntriesFound = false;
    auto& index = boost::multi_index::get<routingtable::tags::ExpiryDate>(multiIndexContainer);
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::system_clock::now().time_since_epoch()).count();
    auto last = index.upper_bound(now);
    std::vector<std::string> expiredParticipantIds;
    for (auto routingEntryIterator = index.lower_bound(0); routingEntryIterator != last;
         ++routingEntryIterator) {
        if (!routingEntryIterator->isSticky) {
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
