/*
 * #%L
 * %%
 * Copyright (C) 2017 - 2018 BMW Car IT GmbH
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
#ifndef CAPABILITIESSTORAGE_H
#define CAPABILITIESSTORAGE_H

#include <string>
#include <vector>

#include <boost/mpl/push_front.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/optional.hpp>

#include <muesli/Traits.h>

#include "joynr/serializer/Serializer.h"
#include "joynr/types/DiscoveryEntry.h"

namespace joynr
{

namespace capabilities
{

using joynr::types::DiscoveryEntry;

namespace tags
{
struct DomainAndInterface;
struct ParticipantId;
struct ExpiryDate;
struct Timestamp;
} // namespace tags

using InterfaceNameKey = BOOST_MULTI_INDEX_CONST_MEM_FUN(DiscoveryEntry,
                                                         const std::string&,
                                                         getInterfaceName);
using DomainKey = BOOST_MULTI_INDEX_CONST_MEM_FUN(DiscoveryEntry, const std::string&, getDomain);
using ParticipantIdKey = BOOST_MULTI_INDEX_CONST_MEM_FUN(DiscoveryEntry,
                                                         const std::string&,
                                                         getParticipantId);

using ExpiryDateKey = BOOST_MULTI_INDEX_CONST_MEM_FUN(DiscoveryEntry,
                                                      const std::int64_t&,
                                                      getExpiryDateMs);

namespace bmi = boost::multi_index;

struct LocalDiscoveryEntry : public DiscoveryEntry {
    LocalDiscoveryEntry() : DiscoveryEntry(), gbids()
    {
    }
    LocalDiscoveryEntry(const DiscoveryEntry& entry,
                        const std::vector<std::string>& gbidsParam = {})
            : DiscoveryEntry(entry), gbids(gbidsParam)
    {
    }
    std::vector<std::string> gbids;

    template <typename Archive>
    void serialize(Archive& archive)
    {
        archive(muesli::BaseClass<joynr::types::DiscoveryEntry>(this), MUESLI_NVP(gbids));
    }
};

using Container = bmi::multi_index_container<
        LocalDiscoveryEntry,
        bmi::indexed_by<
                bmi::hashed_non_unique<
                        bmi::tag<tags::DomainAndInterface>,
                        bmi::composite_key<LocalDiscoveryEntry, DomainKey, InterfaceNameKey>>,
                bmi::hashed_unique<bmi::tag<tags::ParticipantId>, ParticipantIdKey>,
                bmi::ordered_non_unique<bmi::tag<tags::ExpiryDate>, ExpiryDateKey>>>;

using Timestamp = std::chrono::time_point<std::chrono::system_clock>;

struct CachedDiscoveryEntry : public DiscoveryEntry {
    CachedDiscoveryEntry(const DiscoveryEntry& entry, Timestamp timestamp)
            : DiscoveryEntry(entry), _timestamp(timestamp)
    {
    }
    Timestamp _timestamp;
};

using ContainerIndices = Container::index_specifier_type_list;
using SequencedContainerIndices = boost::mpl::push_front<ContainerIndices, bmi::sequenced<>>::type;
using TimestampKey = BOOST_MULTI_INDEX_MEMBER(CachedDiscoveryEntry, Timestamp, _timestamp);
using TimestampIndex = bmi::ordered_non_unique<bmi::tag<tags::Timestamp>, TimestampKey>;
using CacheContainerIndices =
        boost::mpl::push_back<SequencedContainerIndices, TimestampIndex>::type;

using CachingContainer = bmi::multi_index_container<CachedDiscoveryEntry, CacheContainerIndices>;

template <typename C>
class BaseStorage
{
public:
    virtual ~BaseStorage() = default;
    virtual std::vector<DiscoveryEntry> lookupByDomainAndInterface(
            const std::string& domain,
            const std::string& interface) const
    {
        return lookupByDomainAndInterfaceFiltered(domain, interface, NoFilter());
    }

    virtual boost::optional<DiscoveryEntry> lookupByParticipantId(
            const std::string& participantId) const
    {
        return lookupByParticipantIdFiltered(participantId, NoFilter());
    }

    virtual void removeByParticipantId(const std::string& participantId)
    {
        auto& index = _container.template get<tags::ParticipantId>();
        auto it = index.find(participantId);
        if (it != index.end()) {
            index.erase(it);
        }
    }

    virtual void clear()
    {
        _container.clear();
    }

    template <typename Archive>
    void serialize(Archive& archive)
    {
        archive(_container);
    }

    auto begin()
    {
        return _container.begin();
    }

    auto begin() const
    {
        return _container.begin();
    }

    auto cbegin() const
    {
        return _container.cbegin();
    }

    auto end()
    {
        return _container.end();
    }

    auto end() const
    {
        return _container.end();
    }

    auto cend() const
    {
        return _container.cend();
    }

    virtual std::size_t size() const
    {
        return _container.size();
    }

    /**
     * @brief removes expired entries based on expiryDate
     * @return expired/removed entries
     */
    virtual std::vector<DiscoveryEntry> removeExpired()
    {
        auto& index = _container.template get<tags::ExpiryDate>();
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count();
        auto last = index.lower_bound(now);
        std::vector<DiscoveryEntry> removedEntries(index.begin(), last);
        index.erase(index.begin(), last);
        return removedEntries;
    }

    std::vector<std::string> touchAndReturnGlobalParticipantIds(
            const std::int64_t newLastSeenDateMs,
            const std::int64_t newExpiryDateMs)
    {
        std::vector<std::string> result;
        auto modifier = [&result, newLastSeenDateMs, newExpiryDateMs](DiscoveryEntry& entry) {
            bool refresh_entry = false;
            if (newLastSeenDateMs > entry.getLastSeenDateMs()) {
                entry.setLastSeenDateMs(newLastSeenDateMs);
                refresh_entry = true;
            }
            if (newExpiryDateMs > entry.getExpiryDateMs()) {
                entry.setExpiryDateMs(newExpiryDateMs);
                refresh_entry = true;
            }
            if (refresh_entry && entry.getQos().getScope() == types::ProviderScope::GLOBAL) {
                result.push_back(entry.getParticipantId());
            }
        };
        auto& index = _container.template get<tags::ParticipantId>();
        for (auto it = index.begin(); it != index.end(); it++) {
            index.modify(it, modifier);
        }
        return result;
    }

    void touchSelected(const std::vector<std::string> participantIds,
                       const std::int64_t newLastSeenDateMs,
                       const std::int64_t newExpiryDateMs)
    {
        auto modifier = [newLastSeenDateMs, newExpiryDateMs](DiscoveryEntry& entry) {
            if (newLastSeenDateMs > entry.getLastSeenDateMs()) {
                entry.setLastSeenDateMs(newLastSeenDateMs);
            }
            if (newExpiryDateMs > entry.getExpiryDateMs()) {
                entry.setExpiryDateMs(newExpiryDateMs);
            }
        };
        auto& index = _container.template get<tags::ParticipantId>();
        for (const auto& participantId : participantIds) {
            auto it = index.find(participantId);
            if (it != index.end()) {
                index.modify(it, modifier);
            }
        }
    }

protected:
    template <typename FilterFun>
    std::vector<DiscoveryEntry> lookupByDomainAndInterfaceFiltered(const std::string& domain,
                                                                   const std::string& interface,
                                                                   FilterFun filterFun) const
    {
        std::vector<DiscoveryEntry> result;

        auto& index = _container.template get<tags::DomainAndInterface>();
        auto range = index.equal_range(std::tie(domain, interface));
        std::copy_if(range.first, range.second, std::back_inserter(result), filterFun);

        return result;
    }

    template <typename FilterFun>
    boost::optional<DiscoveryEntry> lookupByParticipantIdFiltered(const std::string& participantId,
                                                                  FilterFun filterFun) const
    {
        boost::optional<DiscoveryEntry> result;

        auto& index = _container.template get<tags::ParticipantId>();
        auto it = index.find(participantId);
        if (it != index.end()) {
            if (filterFun(*it)) {
                result = *it;
            }
        }
        return result;
    }

    struct NoFilter {
        template <typename T>
        bool operator()(T&&) const
        {
            return true;
        }
    };

    C _container;
};

class Storage : public BaseStorage<Container>
{
public:
    virtual ~Storage() = default;
    virtual void insert(const DiscoveryEntry& entry, const std::vector<std::string>& gbids = {})
    {
        auto& index = _container.get<tags::ParticipantId>();
        LocalDiscoveryEntry entryWithGbids(entry, gbids);
        auto insertResult = index.insert(entryWithGbids);

        // entry already existed
        if (!insertResult.second) {

            auto existingIt = insertResult.first;

            // replace
            bool replaceResult = index.replace(existingIt, entryWithGbids);
            assert(replaceResult);
            std::ignore = replaceResult;
        }
    }
};

class CachingStorage : public BaseStorage<CachingContainer>
{
private:
    static auto filterByAge(std::chrono::milliseconds maxAge)
    {
        auto now = std::chrono::system_clock::now();
        return [maxAge, now](const CachedDiscoveryEntry& cachedEntry) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - cachedEntry._timestamp);
            return elapsed <= maxAge;
        };
    }

public:
    CachingStorage(std::size_t maxElementCount = 1000) : _maxElementCount(maxElementCount)
    {
    }

    virtual ~CachingStorage() = default;
    virtual void insert(const DiscoveryEntry& entry)
    {
        auto& index = _container.get<tags::ParticipantId>();

        auto now = std::chrono::system_clock::now();
        CachedDiscoveryEntry cachedEntry(entry, now);
        auto insertResult = index.insert(cachedEntry);

        // entry already existed
        if (!insertResult.second) {

            auto existingIt = insertResult.first;

            // replace
            bool replaceResult = index.replace(existingIt, cachedEntry);
            assert(replaceResult);
            std::ignore = replaceResult;

            // rank to top
            auto sequencedIt = _container.project<0>(existingIt);
            _container.relocate(_container.begin(), sequencedIt);
        } else if (_container.size() > _maxElementCount) {
            _container.pop_back();
        }
    }

    virtual boost::optional<DiscoveryEntry> lookupCacheByParticipantId(
            const std::string& participantId,
            std::chrono::milliseconds maxAge) const
    {
        return lookupByParticipantIdFiltered(participantId, filterByAge(maxAge));
    }

    virtual std::vector<DiscoveryEntry> lookupCacheByDomainAndInterface(
            const std::string& domain,
            const std::string& interface,
            std::chrono::milliseconds maxAge) const
    {
        return lookupByDomainAndInterfaceFiltered(domain, interface, filterByAge(maxAge));
    }

private:
    std::size_t _maxElementCount;
};

} // namespace capabilities

} // namespace joynr

MUESLI_REGISTER_TYPE(joynr::capabilities::BaseStorage<joynr::capabilities::Container>,
                     "joynr.capabilities.BaseStorage")
MUESLI_REGISTER_POLYMORPHIC_TYPE(joynr::capabilities::Storage,
                                 joynr::capabilities::BaseStorage<joynr::capabilities::Container>,
                                 "joynr::capabilities::Storage")

namespace muesli
{
template <>
struct SkipIntroOutroTraits<joynr::capabilities::Storage> : std::true_type {
};
} // namespace muesli

#endif // CAPABILITIESSTORAGE_H
