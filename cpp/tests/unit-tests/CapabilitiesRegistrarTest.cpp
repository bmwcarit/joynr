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
#include <memory>
#include <string>
#include <limits>

#include <boost/optional/optional_io.hpp>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/CapabilitiesRegistrar.h"
#include "joynr/Future.h"
#include "joynr/IMessageSender.h"
#include "joynr/MessagingQos.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"
#include "joynr/types/DiscoveryQos.h"
#include "joynr/types/Version.h"

#include "tests/mock/MockDiscovery.h"
#include "tests/mock/MockDispatcher.h"
#include "tests/mock/MockProvider.h"
#include "tests/mock/MockMessageRouter.h"
#include "tests/mock/MockParticipantIdStorage.h"
#include "tests/mock/MockMessageSender.h"

using ::testing::_;
using ::testing::DoAll;
using ::testing::Eq;
using ::testing::InSequence;
using ::testing::InvokeArgument;
using ::testing::Mock;
using ::testing::Property;
using ::testing::Return;
using ::testing::Sequence;

using namespace joynr;

class CapabilitiesRegistrarTest : public ::testing::Test
{
public:
    CapabilitiesRegistrarTest()
            : _mockDispatcher(),
              _dispatcherAddress(),
              _mockParticipantIdStorage(std::make_shared<MockParticipantIdStorage>()),
              _mockDiscovery(std::make_shared<MockDiscovery>()),
              _capabilitiesRegistrar(nullptr),
              _mockProvider(std::make_shared<MockProvider>()),
              _domain("testDomain"),
              _expectedParticipantId("testParticipantId"),
              _singleThreadedIOService(std::make_shared<SingleThreadedIOService>()),
              _mockMessageRouter(
                      std::make_shared<MockMessageRouter>(_singleThreadedIOService->getIOService())),
              _expectedProviderVersion(_mockProvider->MAJOR_VERSION, _mockProvider->MINOR_VERSION),
              _mockMessageSender(std::make_shared<MockMessageSender>()),
              _pubManager(
                      std::make_shared<PublicationManager>(_singleThreadedIOService->getIOService(),
                                                           _mockMessageSender))
    {
        _singleThreadedIOService->start();
    }

    ~CapabilitiesRegistrarTest()
    {
        delete _capabilitiesRegistrar;
        _pubManager->shutdown();
        _singleThreadedIOService->stop();
        _pubManager.reset();
        _mockMessageSender.reset();
    }

    void SetUp()
    {
        std::vector<std::shared_ptr<IDispatcher>> dispatcherList;
        _mockDispatcher = std::make_shared<MockDispatcher>();
        dispatcherList.push_back(_mockDispatcher);

        const std::string globalAddress = "testGlobalAddressString";
        _capabilitiesRegistrar = new CapabilitiesRegistrar(dispatcherList,
                                                          _mockDiscovery,
                                                          _mockParticipantIdStorage,
                                                          _dispatcherAddress,
                                                          _mockMessageRouter,
                                                          std::numeric_limits<std::int64_t>::max(),
                                                          _pubManager,
                                                          globalAddress);
    }

protected:
    DISALLOW_COPY_AND_ASSIGN(CapabilitiesRegistrarTest);
    std::shared_ptr<MockDispatcher> _mockDispatcher;
    std::shared_ptr<const joynr::system::RoutingTypes::Address> _dispatcherAddress;
    std::shared_ptr<MockParticipantIdStorage> _mockParticipantIdStorage;
    std::shared_ptr<MockDiscovery> _mockDiscovery;
    CapabilitiesRegistrar* _capabilitiesRegistrar;
    std::shared_ptr<MockProvider> _mockProvider;
    std::string _domain;
    std::string _expectedParticipantId;
    std::shared_ptr<SingleThreadedIOService> _singleThreadedIOService;
    std::shared_ptr<MockMessageRouter> _mockMessageRouter;
    const types::Version _expectedProviderVersion;
    std::shared_ptr<IMessageSender> _mockMessageSender;
    std::shared_ptr<PublicationManager> _pubManager;
};

TEST_F(CapabilitiesRegistrarTest, add)
{

    types::ProviderQos testQos;
    testQos.setPriority(100);
    EXPECT_CALL(*_mockParticipantIdStorage,
                getProviderParticipantId(_domain, MockProvider::INTERFACE_NAME(), MockProvider::MAJOR_VERSION))
            .Times(1)
            .WillOnce(Return(_expectedParticipantId));
    EXPECT_CALL(*_mockDispatcher, addRequestCaller(_expectedParticipantId, _)).Times(1);
    auto mockFuture = std::make_shared<joynr::Future<void>>();
    mockFuture->onSuccess();
    std::vector<std::string> capturedGbids;
    EXPECT_CALL(*_mockDiscovery,
                addAsyncMock(AllOf(Property(&joynr::types::DiscoveryEntry::getDomain, Eq(_domain)),
                                   Property(&joynr::types::DiscoveryEntry::getInterfaceName,
                                            Eq(MockProvider::INTERFACE_NAME())),
                                   Property(&joynr::types::DiscoveryEntry::getParticipantId,
                                            Eq(_expectedParticipantId)),
                                   Property(&joynr::types::DiscoveryEntry::getQos, Eq(testQos)),
                                   Property(&joynr::types::DiscoveryEntry::getProviderVersion,
                                            Eq(_expectedProviderVersion))),
                             _,
                             _,
                             _,
                             _,
                             _,
                             _)).WillOnce(DoAll(::testing::SaveArg<2>(&capturedGbids), InvokeArgument<3>(), Return(mockFuture)));

    Future<void> future;
    auto onSuccess = [&future]() { future.onSuccess(); };
    auto onError = [&future](const exceptions::JoynrRuntimeException& exception) {
        future.onError(std::make_shared<exceptions::JoynrRuntimeException>(exception));
    };

    std::string participantId =
            _capabilitiesRegistrar->addAsync(_domain, _mockProvider, testQos, onSuccess, onError);
    future.get();

    EXPECT_EQ(_expectedParticipantId, participantId);
    EXPECT_EQ(capturedGbids.size(), 0);
}

TEST_F(CapabilitiesRegistrarTest, checkVisibilityOfGlobalAndLocalProviders)
{

    types::ProviderQos testQos;
    testQos.setScope(types::ProviderScope::GLOBAL);
    EXPECT_CALL(*_mockParticipantIdStorage,
                getProviderParticipantId(_domain, MockProvider::INTERFACE_NAME(), MockProvider::MAJOR_VERSION))
            .Times(2)
            .WillRepeatedly(Return(_expectedParticipantId));

    auto mockFuture = std::make_shared<joynr::Future<void>>();
    mockFuture->onSuccess();
    EXPECT_CALL(*_mockDiscovery, addAsyncMock(_, _, _, _, _, _, _)).Times(2).WillRepeatedly(
            DoAll(InvokeArgument<3>(), Return(mockFuture)));

    ON_CALL(*_mockMessageRouter, addNextHop(_, _, _, _, _, _, _))
            .WillByDefault(InvokeArgument<5>());
    bool expectedIsGloballyVisible = true;
    EXPECT_CALL(*_mockMessageRouter,
                addNextHop(Eq(_expectedParticipantId),
                           Eq(_dispatcherAddress),
                           Eq(expectedIsGloballyVisible),
                           _,
                           _,
                           _,
                           _));

    Future<void> future;
    auto onSuccess = [&future]() { future.onSuccess(); };
    auto onError = [&future](const exceptions::JoynrRuntimeException& exception) {
        future.onError(std::make_shared<exceptions::JoynrRuntimeException>(exception));
    };

    _capabilitiesRegistrar->addAsync(_domain, _mockProvider, testQos, onSuccess, onError);
    future.get();

    Mock::VerifyAndClearExpectations(_mockMessageRouter.get());

    testQos.setScope(types::ProviderScope::LOCAL);

    expectedIsGloballyVisible = false;
    EXPECT_CALL(*_mockMessageRouter,
                addNextHop(Eq(_expectedParticipantId),
                           Eq(_dispatcherAddress),
                           Eq(expectedIsGloballyVisible),
                           _,
                           _,
                           _,
                           _));

    Future<void> future1;
    auto onSuccess1 = [&future1]() { future1.onSuccess(); };
    auto onError1 = [&future1](const exceptions::JoynrRuntimeException& exception) {
        future1.onError(std::make_shared<exceptions::JoynrRuntimeException>(exception));
    };

    _capabilitiesRegistrar->addAsync(_domain, _mockProvider, testQos, onSuccess1, onError1);
    future1.get();
}

TEST_F(CapabilitiesRegistrarTest, removeWithDomainAndProviderObject)
{
    Sequence s;
    EXPECT_CALL(*_mockParticipantIdStorage,
                getProviderParticipantId(_domain, MockProvider::INTERFACE_NAME(), MockProvider::MAJOR_VERSION))
            .Times(1)
            .InSequence(s)
            .WillOnce(Return(_expectedParticipantId));
    auto mockFuture = std::make_shared<joynr::Future<void>>();
    mockFuture->onSuccess();
    EXPECT_CALL(*_mockDiscovery, removeAsyncMock(_expectedParticipantId, _, _, _))
            .Times(1)
            .InSequence(s)
            .WillOnce(DoAll(InvokeArgument<1>(), Return(mockFuture)));
    EXPECT_CALL(*_mockMessageRouter, removeNextHop(Eq(_expectedParticipantId), _, _))
            .Times(1)
            .InSequence(s);
    EXPECT_CALL(*_mockDispatcher, removeRequestCaller(_expectedParticipantId))
            .Times(1)
            .InSequence(s);

    Future<void> future;
    auto onSuccess = [&future]() { future.onSuccess(); };
    auto onError = [&future](const exceptions::JoynrRuntimeException& exception) {
        future.onError(std::make_shared<exceptions::JoynrRuntimeException>(exception));
    };

    std::string participantId =
            _capabilitiesRegistrar->removeAsync(_domain, _mockProvider, onSuccess, onError);
    future.get();

    EXPECT_EQ(_expectedParticipantId, participantId);
}

TEST_F(CapabilitiesRegistrarTest, removeWithParticipantId)
{
    Sequence s;
    auto mockFuture = std::make_shared<joynr::Future<void>>();
    mockFuture->onSuccess();
    EXPECT_CALL(*_mockDiscovery, removeAsyncMock(_expectedParticipantId, _, _, _))
            .Times(1)
            .InSequence(s)
            .WillOnce(DoAll(InvokeArgument<1>(), Return(mockFuture)));
    EXPECT_CALL(*_mockMessageRouter, removeNextHop(Eq(_expectedParticipantId), _, _))
            .Times(1)
            .InSequence(s);
    EXPECT_CALL(*_mockDispatcher, removeRequestCaller(_expectedParticipantId))
            .Times(1)
            .InSequence(s);

    Future<void> future;
    auto onSuccess = [&future]() { future.onSuccess(); };
    auto onError = [&future](const exceptions::JoynrRuntimeException& exception) {
        future.onError(std::make_shared<exceptions::JoynrRuntimeException>(exception));
    };

    _capabilitiesRegistrar->removeAsync(_expectedParticipantId, onSuccess, onError);
    future.get();
}

TEST_F(CapabilitiesRegistrarTest, registerMultipleDispatchersAndRegisterCapability)
{
    auto mockDispatcher1 = std::make_shared<MockDispatcher>();
    auto mockDispatcher2 = std::make_shared<MockDispatcher>();
    types::ProviderQos testQos;
    testQos.setPriority(100);

    EXPECT_CALL(*_mockParticipantIdStorage,
                getProviderParticipantId(_domain, MockProvider::INTERFACE_NAME(), MockProvider::MAJOR_VERSION))
            .Times(1)
            .WillOnce(Return(_expectedParticipantId));

    auto mockFuture = std::make_shared<joynr::Future<void>>();
    mockFuture->onSuccess();

    EXPECT_CALL(*_mockDiscovery,
                addAsyncMock(AllOf(Property(&joynr::types::DiscoveryEntry::getDomain, Eq(_domain)),
                                   Property(&joynr::types::DiscoveryEntry::getInterfaceName,
                                            Eq(MockProvider::INTERFACE_NAME())),
                                   Property(&joynr::types::DiscoveryEntry::getParticipantId,
                                            Eq(_expectedParticipantId)),
                                   Property(&joynr::types::DiscoveryEntry::getQos, Eq(testQos))),
                             _,
                             _,
                             _,
                             _,
                             _,
                             _))
            .Times(1)
            .WillOnce(DoAll(InvokeArgument<3>(), Return(mockFuture)));

    EXPECT_CALL(*_mockDispatcher, addRequestCaller(_expectedParticipantId, _)).Times(1);
    EXPECT_CALL(*mockDispatcher1, addRequestCaller(_expectedParticipantId, _)).Times(1);
    EXPECT_CALL(*mockDispatcher2, addRequestCaller(_expectedParticipantId, _)).Times(1);

    _capabilitiesRegistrar->addDispatcher(mockDispatcher1);
    _capabilitiesRegistrar->addDispatcher(mockDispatcher2);

    Future<void> future;
    auto onSuccess = [&future]() { future.onSuccess(); };
    auto onError = [&future](const exceptions::JoynrRuntimeException& exception) {
        future.onError(std::make_shared<exceptions::JoynrRuntimeException>(exception));
    };

    std::string participantId =
            _capabilitiesRegistrar->addAsync(_domain, _mockProvider, testQos, onSuccess, onError);
    future.get();

    EXPECT_EQ(_expectedParticipantId, participantId);
}

TEST_F(CapabilitiesRegistrarTest, removeDispatcher)
{
    auto mockDispatcher1 = std::make_shared<MockDispatcher>();
    auto mockDispatcher2 = std::make_shared<MockDispatcher>();
    types::ProviderQos testQos;
    testQos.setPriority(100);

    _capabilitiesRegistrar->addDispatcher(mockDispatcher1);
    _capabilitiesRegistrar->addDispatcher(mockDispatcher2);

    _capabilitiesRegistrar->removeDispatcher(mockDispatcher1);

    EXPECT_CALL(*_mockParticipantIdStorage,
                getProviderParticipantId(_domain, MockProvider::INTERFACE_NAME(), MockProvider::MAJOR_VERSION))
            .Times(1)
            .WillOnce(Return(_expectedParticipantId));

    auto mockFuture = std::make_shared<joynr::Future<void>>();
    mockFuture->onSuccess();
    EXPECT_CALL(*_mockDiscovery,
                addAsyncMock(AllOf(Property(&joynr::types::DiscoveryEntry::getDomain, Eq(_domain)),
                                   Property(&joynr::types::DiscoveryEntry::getInterfaceName,
                                            Eq(MockProvider::INTERFACE_NAME())),
                                   Property(&joynr::types::DiscoveryEntry::getParticipantId,
                                            Eq(_expectedParticipantId)),
                                   Property(&joynr::types::DiscoveryEntry::getQos, Eq(testQos))),
                             _,
                             _,
                             _,
                             _,
                             _,
                             _))
            .Times(1)
            .WillOnce(DoAll(InvokeArgument<3>(), Return(mockFuture)));

    EXPECT_CALL(*_mockDispatcher, addRequestCaller(_expectedParticipantId, _)).Times(1);
    // mockDispatcher1 should not be used as it was removed
    EXPECT_CALL(*mockDispatcher1, addRequestCaller(_expectedParticipantId, _)).Times(0);
    EXPECT_CALL(*mockDispatcher2, addRequestCaller(_expectedParticipantId, _)).Times(1);

    Future<void> future;
    auto onSuccess = [&future]() { future.onSuccess(); };
    auto onError = [&future](const exceptions::JoynrRuntimeException& exception) {
        future.onError(std::make_shared<exceptions::JoynrRuntimeException>(exception));
    };

    std::string participantId =
            _capabilitiesRegistrar->addAsync(_domain, _mockProvider, testQos, onSuccess, onError);
    future.get();

    EXPECT_EQ(_expectedParticipantId, participantId);
}

TEST_F(CapabilitiesRegistrarTest, addWithGbids)
{

    types::ProviderQos testQos;
    testQos.setPriority(100);
    EXPECT_CALL(*_mockParticipantIdStorage,
                getProviderParticipantId(_domain, MockProvider::INTERFACE_NAME(), MockProvider::MAJOR_VERSION))
            .Times(1)
            .WillOnce(Return(_expectedParticipantId));
    EXPECT_CALL(*_mockDispatcher, addRequestCaller(_expectedParticipantId, _)).Times(1);
    auto mockFuture = std::make_shared<joynr::Future<void>>();
    mockFuture->onSuccess();

    std::vector<std::string> capturedGbids;
    EXPECT_CALL(*_mockDiscovery,
                addAsyncMock(AllOf(Property(&joynr::types::DiscoveryEntry::getDomain, Eq(_domain)),
                                   Property(&joynr::types::DiscoveryEntry::getInterfaceName,
                                            Eq(MockProvider::INTERFACE_NAME())),
                                   Property(&joynr::types::DiscoveryEntry::getParticipantId,
                                            Eq(_expectedParticipantId)),
                                   Property(&joynr::types::DiscoveryEntry::getQos, Eq(testQos)),
                                   Property(&joynr::types::DiscoveryEntry::getProviderVersion,
                                            Eq(_expectedProviderVersion))), // discoveryEntry
                             _, // awaitGlobalRegistration
                             _, // gbids
                             _, // onSuccess callback
                             _, // onApplicationError callback
                             _, // onError callback
                             _ // messagingQos
                             )).WillOnce(DoAll(::testing::SaveArg<2>(&capturedGbids), InvokeArgument<3>(), Return(mockFuture)));

    Future<void> future;
    auto onSuccess = [&future]() { future.onSuccess(); };
    auto onError = [&future](const exceptions::JoynrRuntimeException& exception) {
        future.onError(std::make_shared<exceptions::JoynrRuntimeException>(exception));
    };

    bool persist = false;
    bool awaitGlobalRegistration = false;
    bool addToAll = false;
    std::vector<std::string> gbids = { "joynrdefaultgbid", "othergbid" };

    std::string participantId =
            _capabilitiesRegistrar->addAsync(_domain, _mockProvider, testQos, onSuccess, onError, persist, awaitGlobalRegistration, addToAll, gbids);
    future.get();

    EXPECT_EQ(_expectedParticipantId, participantId);
    EXPECT_EQ(gbids, capturedGbids);
}

TEST_F(CapabilitiesRegistrarTest, addToAll)
{

    types::ProviderQos testQos;
    testQos.setPriority(100);
    EXPECT_CALL(*_mockParticipantIdStorage,
                getProviderParticipantId(_domain, MockProvider::INTERFACE_NAME(), MockProvider::MAJOR_VERSION))
            .Times(1)
            .WillOnce(Return(_expectedParticipantId));
    EXPECT_CALL(*_mockDispatcher, addRequestCaller(_expectedParticipantId, _)).Times(1);
    auto mockFuture = std::make_shared<joynr::Future<void>>();
    mockFuture->onSuccess();

    EXPECT_CALL(*_mockDiscovery,
                addToAllAsyncMock(AllOf(Property(&joynr::types::DiscoveryEntry::getDomain, Eq(_domain)),
                                   Property(&joynr::types::DiscoveryEntry::getInterfaceName,
                                            Eq(MockProvider::INTERFACE_NAME())),
                                   Property(&joynr::types::DiscoveryEntry::getParticipantId,
                                            Eq(_expectedParticipantId)),
                                   Property(&joynr::types::DiscoveryEntry::getQos, Eq(testQos)),
                                   Property(&joynr::types::DiscoveryEntry::getProviderVersion,
                                            Eq(_expectedProviderVersion))), // discoveryEntry
                             _, // awaitGlobalRegistration
                             _, // onSuccess callback
                             _, // onApplicationError callback
                             _, // onError callback
                             _ // messagingQos
                             )).WillOnce(DoAll(InvokeArgument<2>(), Return(mockFuture)));

    Future<void> future;
    auto onSuccess = [&future]() { future.onSuccess(); };
    auto onError = [&future](const exceptions::JoynrRuntimeException& exception) {
        future.onError(std::make_shared<exceptions::JoynrRuntimeException>(exception));
    };

    bool persist = false;
    bool awaitGlobalRegistration = false;
    bool addToAll = true;

    std::string participantId =
            _capabilitiesRegistrar->addAsync(_domain, _mockProvider, testQos, onSuccess, onError, persist, awaitGlobalRegistration, addToAll);
    future.get();

    EXPECT_EQ(_expectedParticipantId, participantId);
}

