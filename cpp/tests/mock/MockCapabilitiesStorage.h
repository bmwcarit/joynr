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
#ifndef TESTS_MOCK_MOCKCAPABILITIESSTORAGE_H
#define TESTS_MOCK_MOCKCAPABILITIESSTORAGE_H

#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <boost/optional.hpp>

#include "joynr/CapabilitiesStorage.h"
#include "joynr/types/DiscoveryEntry.h"

namespace joynr
{

namespace capabilities
{

class MockStorage : public Storage
{
private:
    boost::optional<DiscoveryEntry> _result;

public:
    // call it from the test class before calling lookupByParticipantId
    void setLookupByParticipantIdResult(boost::optional<DiscoveryEntry>& result)
    {
        _result = result;
    }

    // from BaseStorage
    boost::optional<DiscoveryEntry> lookupByParticipantId(const std::string& participantId) const override
    {
        lookupByParticipantIdMock(participantId);
        return _result;
    }
    MOCK_CONST_METHOD2(lookupByDomainAndInterface, std::vector<DiscoveryEntry> (const std::string& domain, const std::string& interface));
    MOCK_CONST_METHOD1(lookupByParticipantIdMock, void (const std::string& participantId));
    MOCK_METHOD1(removeByParticipantId, void (const std::string& participantId));
    MOCK_METHOD0(clear, void ());
    MOCK_CONST_METHOD0(size, std::size_t());
    MOCK_METHOD0(removeExpired, std::vector<DiscoveryEntry> ());

    void insert(const DiscoveryEntry& entry, const std::vector<std::string>& gbids = {}) override
    {
        insertMock(entry, gbids);
    }
    MOCK_METHOD2(insertMock , void (const DiscoveryEntry& entry, const std::vector<std::string>& gbids));

    template <typename Archive>
    void serialize(Archive& archive)
    {
        archive(muesli::BaseClass<joynr::capabilities::Storage>(this));
    }
};

class MockCachingStorage : public CachingStorage
{
private:
        boost::optional<DiscoveryEntry> _result;
public:
    // call it from the test class before calling both:
    // lookupByParticipantId and lookupCacheByParticipantId
    void setLookupByParticipantIdResult(boost::optional<DiscoveryEntry>& result)
    {
        _result = result;
    }

    // from BaseStorage
    boost::optional<DiscoveryEntry> lookupByParticipantId(const std::string& participantId) const
    {
        lookupByParticipantIdMock(participantId);
        return _result;
    }
    boost::optional<DiscoveryEntry> lookupCacheByParticipantId(const std::string& participantId, std::chrono::milliseconds maxAge) const {
        lookupCacheByParticipantIdMock(participantId, maxAge);
        return _result;
    }
    MOCK_CONST_METHOD2(lookupByDomainAndInterface, std::vector<DiscoveryEntry> (const std::string& domain, const std::string& interface));
    MOCK_CONST_METHOD1(lookupByParticipantIdMock, void (const std::string& participantId));
    MOCK_METHOD1(removeByParticipantId, void (const std::string& participantId));
    MOCK_METHOD0(clear, void ());
    MOCK_CONST_METHOD0(size, std::size_t());
    MOCK_METHOD0(removeExpired, std::vector<DiscoveryEntry> ());

    MOCK_METHOD1(insert, void (const DiscoveryEntry& entry));
    MOCK_CONST_METHOD2(lookupCacheByParticipantIdMock, void (const std::string& participantId, std::chrono::milliseconds maxAge));
    MOCK_CONST_METHOD3(lookupCacheByDomainAndInterface, std::vector<DiscoveryEntry> (const std::string& domain, const std::string& interface, std::chrono::milliseconds maxAge));
};

} // namespace capabilities

} // namespace joynr

MUESLI_REGISTER_POLYMORPHIC_TYPE(joynr::capabilities::MockStorage,
                                 joynr::capabilities::Storage,
                                 "joynr::capabilities::MockStorage")

#endif // TESTS_MOCK_MOCKCAPABILITIESSTORAGE_H
