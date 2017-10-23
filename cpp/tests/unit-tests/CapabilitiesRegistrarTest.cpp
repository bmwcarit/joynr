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
#include "tests/mock/MockObjects.h"
#include "joynr/types/Version.h"
#include "joynr/IMessageSender.h"
#include "joynr/SingleThreadedIOService.h"

using ::testing::DoAll;
using ::testing::InvokeArgument;
using ::testing::Mock;

using namespace joynr;

class CapabilitiesRegistrarTest : public ::testing::Test {
public:
    CapabilitiesRegistrarTest() :
            mockDispatcher(),
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
            mockMessageSender(std::make_shared<MockMessageSender>()),
            pubManager(std::make_shared<PublicationManager>(singleThreadedIOService.getIOService(), mockMessageSender))
    {
        singleThreadedIOService.start();
    }

    ~CapabilitiesRegistrarTest()
    {
        delete capabilitiesRegistrar;
        pubManager->shutdown();
        singleThreadedIOService.stop();
        pubManager.reset();
        mockMessageSender.reset();
    }

    void SetUp(){
        std::vector<std::shared_ptr<IDispatcher>> dispatcherList;
        mockDispatcher = std::make_shared<MockDispatcher>();
        dispatcherList.push_back(mockDispatcher);

        const std::string globalAddress = "testGlobalAddressString";
        capabilitiesRegistrar = new CapabilitiesRegistrar(
                    dispatcherList,
                    mockDiscovery,
                    mockParticipantIdStorage,
                    dispatcherAddress,
                    mockMessageRouter,
                    std::numeric_limits<std::int64_t>::max(),
                    pubManager,
                    globalAddress
        );
    }

protected:
    DISALLOW_COPY_AND_ASSIGN(CapabilitiesRegistrarTest);
    std::shared_ptr<MockDispatcher> mockDispatcher;
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
    std::shared_ptr<IMessageSender> mockMessageSender;
    std::shared_ptr<PublicationManager> pubManager;
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

TEST_F(CapabilitiesRegistrarTest, checkVisibilityOfGlobalAndLocalProviders){

    types::ProviderQos testQos;
    testQos.setScope(types::ProviderScope::GLOBAL);
    EXPECT_CALL(*mockParticipantIdStorage, getProviderParticipantId(
                    domain,
                    IMockProviderInterface::INTERFACE_NAME()
    ))
            .Times(2)
            .WillRepeatedly(Return(expectedParticipantId));

    auto mockFuture = std::make_shared<joynr::Future<void>>();
    mockFuture->onSuccess();
    EXPECT_CALL(
                mockDiscovery,
                addAsync(
                    _,
                    _,
                    _
                )
    ).Times(2).WillRepeatedly(
                DoAll(InvokeArgument<1>(),
                      Return(mockFuture)
                      )
                );

    ON_CALL(*mockMessageRouter, addNextHop(_,_,_,_,_,_,_)).WillByDefault(InvokeArgument<5>());
    bool expectedIsGloballyVisible = true;
    EXPECT_CALL(*mockMessageRouter, addNextHop(Eq(expectedParticipantId),Eq(dispatcherAddress),Eq(expectedIsGloballyVisible),_,_,_,_));

    Future<void> future;
    auto onSuccess = [&future]() { future.onSuccess(); };
    auto onError = [&future](const exceptions::JoynrRuntimeException& exception) {
        future.onError(std::make_shared<exceptions::JoynrRuntimeException>(exception));
    };

    capabilitiesRegistrar->addAsync(domain, mockProvider, testQos, onSuccess, onError);
    future.get();

    Mock::VerifyAndClearExpectations(mockMessageRouter.get());

    testQos.setScope(types::ProviderScope::LOCAL);

    expectedIsGloballyVisible = false;
    EXPECT_CALL(*mockMessageRouter, addNextHop(Eq(expectedParticipantId),Eq(dispatcherAddress),Eq(expectedIsGloballyVisible),_,_,_,_));

    Future<void> future1;
    auto onSuccess1 = [&future1]() { future1.onSuccess(); };
    auto onError1 = [&future1](const exceptions::JoynrRuntimeException& exception) {
        future1.onError(std::make_shared<exceptions::JoynrRuntimeException>(exception));
    };

    capabilitiesRegistrar->addAsync(domain, mockProvider, testQos, onSuccess1, onError1);
    future1.get();
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
    auto mockDispatcher1 = std::make_shared<MockDispatcher>();
    auto mockDispatcher2 = std::make_shared<MockDispatcher>();
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
}

TEST_F(CapabilitiesRegistrarTest, removeDispatcher){
    auto mockDispatcher1 = std::make_shared<MockDispatcher>();
    auto mockDispatcher2 = std::make_shared<MockDispatcher>();
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
}
