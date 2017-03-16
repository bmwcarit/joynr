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

#include "joynr/PrivateCopyAssign.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "joynr/CapabilitiesRegistrar.h"
#include "tests/utils/MockObjects.h"
#include "joynr/types/Version.h"
#include "joynr/IJoynrMessageSender.h"
#include "joynr/SingleThreadedIOService.h"

using ::testing::DoAll;
using ::testing::InvokeArgument;

using namespace joynr;

class CapabilitiesRegistrarTest : public ::testing::Test {
public:
    CapabilitiesRegistrarTest() :
            mockDispatcher(nullptr),
            dispatcherAddress(),
            mockParticipantIdStorage(new MockParticipantIdStorage()),
            mockDiscovery(),
            capabilitiesRegistrar(nullptr),
            mockProvider(new MockProvider()),
            domain("testDomain"),
            expectedParticipantId("testParticipantId"),
            singleThreadedIOService(),
            mockMessageRouter(new MockMessageRouter(singleThreadedIOService.getIOService())),
            expectedProviderVersion(mockProvider->MAJOR_VERSION, mockProvider->MINOR_VERSION),
            mockJoynrMessageSender(new MockJoynrMessageSender()),
            pubManager(singleThreadedIOService.getIOService(), mockJoynrMessageSender)
    {
        singleThreadedIOService.start();
    }

    void SetUp(){
        std::vector<IDispatcher*> dispatcherList;
        mockDispatcher = new MockDispatcher();
        dispatcherList.push_back(mockDispatcher);

        capabilitiesRegistrar = new CapabilitiesRegistrar(
                    dispatcherList,
                    mockDiscovery,
                    mockParticipantIdStorage,
                    dispatcherAddress,
                    mockMessageRouter,
                    std::numeric_limits<std::int64_t>::max(),
                    pubManager
        );
    }

    void TearDown(){
        delete capabilitiesRegistrar;
        delete mockDispatcher;
        delete mockJoynrMessageSender;
    }

protected:
    DISALLOW_COPY_AND_ASSIGN(CapabilitiesRegistrarTest);
    MockDispatcher* mockDispatcher;
    std::shared_ptr<const joynr::system::RoutingTypes::Address> dispatcherAddress;
    std::shared_ptr<MockParticipantIdStorage> mockParticipantIdStorage;
    MockDiscovery mockDiscovery;
    CapabilitiesRegistrar* capabilitiesRegistrar;
    std::shared_ptr<MockProvider> mockProvider;
    std::string domain;
    std::string expectedParticipantId;
    SingleThreadedIOService singleThreadedIOService;
    std::shared_ptr<MockMessageRouter> mockMessageRouter;
    const types::Version expectedProviderVersion;
    IJoynrMessageSender* mockJoynrMessageSender;
    PublicationManager pubManager;
};

TEST_F(CapabilitiesRegistrarTest, add){

    types::ProviderQos testQos;
    testQos.setPriority(100);
    EXPECT_CALL(*mockParticipantIdStorage, getProviderParticipantId(
                    domain,
                    IMockProviderInterface::INTERFACE_NAME()
    ))
            .Times(1)
            .WillOnce(Return(expectedParticipantId));
    EXPECT_CALL(*mockDispatcher, addRequestCaller(expectedParticipantId,_))
            .Times(1);
    auto mockFuture = std::make_shared<joynr::Future<void>>();
    mockFuture->onSuccess();
    EXPECT_CALL(
                mockDiscovery,
                addAsync(
                    AllOf(
                        Property(&joynr::types::DiscoveryEntry::getDomain, Eq(domain)),
                        Property(&joynr::types::DiscoveryEntry::getInterfaceName, Eq(IMockProviderInterface::INTERFACE_NAME())),
                        Property(&joynr::types::DiscoveryEntry::getParticipantId, Eq(expectedParticipantId)),
                        Property(&joynr::types::DiscoveryEntry::getQos, Eq(testQos)),
                        Property(&joynr::types::DiscoveryEntry::getProviderVersion, Eq(expectedProviderVersion))
                    ),
                    _,
                    _
                )
    ).WillOnce(
                DoAll(InvokeArgument<1>(),
                      Return(mockFuture)
                      )
                );

    Future<void> future;
    auto onSuccess = [&future]() { future.onSuccess(); };
    auto onError = [&future](const exceptions::JoynrRuntimeException& exception) {
        future.onError(std::make_shared<exceptions::JoynrRuntimeException>(exception));
    };

    std::string participantId = capabilitiesRegistrar->addAsync(domain, mockProvider, testQos, onSuccess, onError);
    future.get();

    EXPECT_EQ(expectedParticipantId, participantId);
}

TEST_F(CapabilitiesRegistrarTest, removeWithDomainAndProviderObject){
    EXPECT_CALL(*mockParticipantIdStorage, getProviderParticipantId(
                    domain,
                    IMockProviderInterface::INTERFACE_NAME()
    ))
            .Times(1)
            .WillOnce(Return(expectedParticipantId));
    EXPECT_CALL(*mockDispatcher, removeRequestCaller(expectedParticipantId))
            .Times(1);
    auto mockFuture = std::make_shared<joynr::Future<void>>();
    mockFuture->onSuccess();
    EXPECT_CALL(mockDiscovery, removeAsync(
                    expectedParticipantId,
                    _,
                    _
    ))
            .Times(1)
            .WillOnce(
                DoAll(InvokeArgument<1>(),
                      Return(mockFuture)
                      )
                );

    Future<void> future;
    auto onSuccess = [&future]() { future.onSuccess(); };
    auto onError = [&future](const exceptions::JoynrRuntimeException& exception) {
        future.onError(std::make_shared<exceptions::JoynrRuntimeException>(exception));
    };

    std::string participantId = capabilitiesRegistrar->removeAsync(domain, mockProvider, onSuccess, onError);
    future.get();

    EXPECT_EQ(expectedParticipantId, participantId);
}

TEST_F(CapabilitiesRegistrarTest, removeWithParticipantId){
    EXPECT_CALL(*mockDispatcher, removeRequestCaller(expectedParticipantId))
            .Times(1);

    auto mockFuture = std::make_shared<joynr::Future<void>>();
    mockFuture->onSuccess();
    EXPECT_CALL(mockDiscovery, removeAsync(
                    expectedParticipantId,
                    _,
                    _
    ))
            .Times(1)
            .WillOnce(
                DoAll(InvokeArgument<1>(),
                      Return(mockFuture)
                      )
                );

    Future<void> future;
    auto onSuccess = [&future]() { future.onSuccess(); };
    auto onError = [&future](const exceptions::JoynrRuntimeException& exception) {
        future.onError(std::make_shared<exceptions::JoynrRuntimeException>(exception));
    };

    capabilitiesRegistrar->removeAsync(expectedParticipantId, onSuccess, onError);
    future.get();
}

TEST_F(CapabilitiesRegistrarTest, registerMultipleDispatchersAndRegisterCapability){
    MockDispatcher* mockDispatcher1 = new MockDispatcher();
    MockDispatcher* mockDispatcher2 = new MockDispatcher();
    types::ProviderQos testQos;
    testQos.setPriority(100);

    EXPECT_CALL(*mockParticipantIdStorage, getProviderParticipantId(
                    domain,
                    IMockProviderInterface::INTERFACE_NAME()
    ))
            .Times(1)
            .WillOnce(Return(expectedParticipantId));

    auto mockFuture = std::make_shared<joynr::Future<void>>();
    mockFuture->onSuccess();

    EXPECT_CALL(
                mockDiscovery,
                addAsync(
                    AllOf(
                        Property(&joynr::types::DiscoveryEntry::getDomain, Eq(domain)),
                        Property(&joynr::types::DiscoveryEntry::getInterfaceName, Eq(IMockProviderInterface::INTERFACE_NAME())),
                        Property(&joynr::types::DiscoveryEntry::getParticipantId, Eq(expectedParticipantId)),
                        Property(&joynr::types::DiscoveryEntry::getQos, Eq(testQos))
                    ),
                    _,
                    _
                )
    ).Times(1).WillOnce(
                DoAll(InvokeArgument<1>(),
                      Return(mockFuture)
                      )
                );

    EXPECT_CALL(*mockDispatcher, addRequestCaller(expectedParticipantId,_))
            .Times(1);
    EXPECT_CALL(*mockDispatcher1, addRequestCaller(expectedParticipantId,_))
            .Times(1);
    EXPECT_CALL(*mockDispatcher2, addRequestCaller(expectedParticipantId,_))
            .Times(1);

    capabilitiesRegistrar->addDispatcher(mockDispatcher1);
    capabilitiesRegistrar->addDispatcher(mockDispatcher2);

    Future<void> future;
    auto onSuccess = [&future]() { future.onSuccess(); };
    auto onError = [&future](const exceptions::JoynrRuntimeException& exception) {
        future.onError(std::make_shared<exceptions::JoynrRuntimeException>(exception));
    };

    std::string participantId = capabilitiesRegistrar->addAsync(domain, mockProvider, testQos, onSuccess, onError);
    future.get();

    EXPECT_EQ(expectedParticipantId, participantId);

    delete mockDispatcher1;
    delete mockDispatcher2;
}

TEST_F(CapabilitiesRegistrarTest, removeDispatcher){
    MockDispatcher* mockDispatcher1 = new MockDispatcher();
    MockDispatcher* mockDispatcher2 = new MockDispatcher();
    types::ProviderQos testQos;
    testQos.setPriority(100);

    capabilitiesRegistrar->addDispatcher(mockDispatcher1);
    capabilitiesRegistrar->addDispatcher(mockDispatcher2);

    capabilitiesRegistrar->removeDispatcher(mockDispatcher1);

    EXPECT_CALL(*mockParticipantIdStorage, getProviderParticipantId(
                    domain,
                    IMockProviderInterface::INTERFACE_NAME()
    ))
            .Times(1)
            .WillOnce(Return(expectedParticipantId));

    auto mockFuture = std::make_shared<joynr::Future<void>>();
    mockFuture->onSuccess();
    EXPECT_CALL(
                mockDiscovery,
                addAsync(
                    AllOf(
                        Property(&joynr::types::DiscoveryEntry::getDomain, Eq(domain)),
                        Property(&joynr::types::DiscoveryEntry::getInterfaceName, Eq(IMockProviderInterface::INTERFACE_NAME())),
                        Property(&joynr::types::DiscoveryEntry::getParticipantId, Eq(expectedParticipantId)),
                        Property(&joynr::types::DiscoveryEntry::getQos, Eq(testQos))
                    ),
                    _,
                    _
                )
    ).Times(1).WillOnce(
                DoAll(InvokeArgument<1>(),
                      Return(mockFuture)
                      )
                );

    EXPECT_CALL(*mockDispatcher, addRequestCaller(expectedParticipantId,_))
            .Times(1);
    //mockDispatcher1 should not be used as it was removed
    EXPECT_CALL(*mockDispatcher1, addRequestCaller(expectedParticipantId,_))
            .Times(0);
    EXPECT_CALL(*mockDispatcher2, addRequestCaller(expectedParticipantId,_))
            .Times(1);

    Future<void> future;
    auto onSuccess = [&future]() { future.onSuccess(); };
    auto onError = [&future](const exceptions::JoynrRuntimeException& exception) {
        future.onError(std::make_shared<exceptions::JoynrRuntimeException>(exception));
    };

    std::string participantId = capabilitiesRegistrar->addAsync(domain, mockProvider, testQos, onSuccess, onError);
    future.get();

    EXPECT_EQ(expectedParticipantId, participantId);

    delete mockDispatcher1;
    delete mockDispatcher2;
}
