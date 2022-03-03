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
#ifndef ROUTINGTABLE_H
#define ROUTINGTABLE_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <limits>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/system/error_code.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/optional.hpp>

#include "joynr/InProcessMessagingAddress.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/system/RoutingTypes/Address.h"

namespace joynr
{

namespace routingtable
{
namespace tags
{
// tags for accessing indices
struct ParticipantId;
struct Address;
struct ExpiryDate;
} // namespace tags

// record to be stored in multi index
struct RoutingEntry
{
    RoutingEntry()
            : participantId(),
              address(),
              isGloballyVisible(true),
              _expiryDateMs(std::numeric_limits<std::int64_t>::max()),
              _isSticky(false)
    {
    }

    explicit RoutingEntry(std::string participantIdLocal,
                          std::shared_ptr<const joynr::system::RoutingTypes::Address> addressLocal,
                          bool isGloballyVisibleLocal,
                          std::int64_t expiryDateMs,
                          bool isSticky)
            : participantId(std::move(participantIdLocal)),
              address(std::move(addressLocal)),
              isGloballyVisible(isGloballyVisibleLocal),
              _expiryDateMs(std::move(expiryDateMs)),
              _isSticky(std::move(isSticky))
    {
    }

    RoutingEntry(RoutingEntry&&) = default;
    RoutingEntry(const RoutingEntry&) = default;
    RoutingEntry& operator=(RoutingEntry&&) = default;
    RoutingEntry& operator=(const RoutingEntry&) = default;

    /**
     * @brief "equal to" operator
     * @param other reference to the object to compare to
     * @return true if objects are equal, false otherwise
     */
    bool operator==(const RoutingEntry& other) const
    {
        if (this->participantId != other.participantId) {
            return false;
        }
        if (this->isGloballyVisible != other.isGloballyVisible) {
            return false;
        }
        if (this->_expiryDateMs != other._expiryDateMs) {
            return false;
        }
        if (this->_isSticky != other._isSticky) {
            return false;
        }
        if (*(this->address) != *(other.address)) {
            return false;
        }
        return true;
    }

    template <typename Archive>
    void serialize(Archive& archive)
    {
        archive(MUESLI_NVP(participantId), MUESLI_NVP(address), MUESLI_NVP(isGloballyVisible));
    }

    std::size_t hashCode() const
    {
        return address->hashCode();
    }

    std::string toString() const
    {
        return joynr::serializer::serializeToJson(*this);
    }

    std::string participantId;
    std::shared_ptr<const joynr::system::RoutingTypes::Address> address;
    bool isGloballyVisible;
    std::int64_t _expiryDateMs;
    bool _isSticky; // true if entry should be protected from being purged

    friend void PrintTo(const RoutingEntry& routingEntry, ::std::ostream* os);
};

void PrintTo(const RoutingEntry& messagingQos, ::std::ostream* os);

std::ostream& operator<<(std::ostream& os, const RoutingEntry& messagingQos);

} // namespace routingtable

class RoutingTable
{

public:
    RoutingTable(const std::string& gcdParticipantId, const std::vector<std::string>& knownGbids);
    ~RoutingTable();

    /*
     * Returns the element with the given participantId. In case the element could not be found
     * nullptr is returned.
     */
    boost::optional<routingtable::RoutingEntry> lookupRoutingEntryByParticipantId(
            const std::string& participantId) const;

    boost::optional<routingtable::RoutingEntry> lookupRoutingEntryByParticipantIdAndGbid(
            const std::string& participantId,
            const std::string& gbid) const;

    /*
     * Returns the elements with the given address.
     */
    std::unordered_set<std::string> lookupParticipantIdsByAddress(
            std::shared_ptr<const joynr::system::RoutingTypes::Address> searchValue) const;

    /*
     * Returns true if an element with the given participantId could be found. False otherwise.
     */
    bool containsParticipantId(const std::string& participantId) const;

    /*
     * Adds an element and keeps it until actively removed (using the 'remove' method)
     */
    void add(const std::string& participantId,
             bool isGloballyVisible,
             std::shared_ptr<const joynr::system::RoutingTypes::Address> address,
             std::int64_t expiryDateMs,
             bool isSticky);

    /*
     * Remove element identified by participantId
     */
    void remove(const std::string& participantId);

    /*
     * Remove expired elements
     */
    void purge();

    template <typename Archive>
    void save(Archive& archive)
    {
        MultiIndexContainer tempMultiIndexContainer;
        auto& index =
                boost::multi_index::get<routingtable::tags::ParticipantId>(_multiIndexContainer);
        auto entry = index.cbegin();
        auto last = index.cend();
        for (; entry != last; ++entry) {
            const joynr::InProcessMessagingAddress* inprocessAddress =
                    dynamic_cast<const joynr::InProcessMessagingAddress*>(entry->address.get());
            if (inprocessAddress == nullptr) {
                tempMultiIndexContainer.insert(*entry);
            }
        }
        archive(tempMultiIndexContainer);
    }

    template <typename Archive>
    void load(Archive& archive)
    {
        archive(_multiIndexContainer);
    }

private:
    struct AddressEqual
            : std::binary_function<std::shared_ptr<const joynr::system::RoutingTypes::Address>,
                                   std::shared_ptr<const joynr::system::RoutingTypes::Address>,
                                   bool>
    {
    public:
        bool operator()(std::shared_ptr<const joynr::system::RoutingTypes::Address> lhs,
                        std::shared_ptr<const joynr::system::RoutingTypes::Address> rhs) const;
    };

    struct AddressHash
            : std::unary_function<std::shared_ptr<const joynr::system::RoutingTypes::Address>,
                                  std::size_t>
    {
    public:
        size_t operator()(
                std::shared_ptr<const joynr::system::RoutingTypes::Address> address) const;
    };

    using MultiIndexContainer = boost::multi_index_container<
            routingtable::RoutingEntry,
            boost::multi_index::indexed_by<
                    boost::multi_index::hashed_unique<
                            boost::multi_index::tag<routingtable::tags::ParticipantId>,
                            BOOST_MULTI_INDEX_MEMBER(routingtable::RoutingEntry,
                                                     std::string,
                                                     participantId)>,
                    boost::multi_index::hashed_non_unique<
                            boost::multi_index::tag<routingtable::tags::Address>,
                            BOOST_MULTI_INDEX_MEMBER(
                                    routingtable::RoutingEntry,
                                    std::shared_ptr<const joynr::system::RoutingTypes::Address>,
                                    address),
                            AddressHash,
                            AddressEqual>,
                    boost::multi_index::ordered_non_unique<
                            boost::multi_index::tag<routingtable::tags::ExpiryDate>,
                            BOOST_MULTI_INDEX_MEMBER(routingtable::RoutingEntry,
                                                     std::int64_t,
                                                     _expiryDateMs)>>>;

private:
    DISALLOW_COPY_AND_ASSIGN(RoutingTable);
    MultiIndexContainer _multiIndexContainer;
    std::string _gcdParticipantId;
    std::unordered_set<std::string> _knownGbidsSet;
    ADD_LOGGER(RoutingTable)
};

} // namespace joynr

namespace muesli
{
template <>
struct SkipIntroOutroTraits<joynr::RoutingTable> : std::true_type
{
};
} // namespace muesli

#endif // ROUTINGTABLE_H
