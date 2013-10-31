/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
#include "joynr/PrivateCopyAssign.h"
#include "PrettyPrint.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "utils/TestQString.h"
#include "joynr/CapabilitiesRegistrar.h"
#include "tests/utils/MockObjects.h"
#include "joynr/types/ProviderQos.h"

using namespace ::testing;
using namespace joynr;

const QString participantIdFile("test_participantids.settings");
const QString authToken("test_authtoken");

class CapabilitiesRegistrarTest : public ::testing::Test {
public:
    CapabilitiesRegistrarTest()
        : mockDispatcher(NULL),
          mockCapabilitiesStub(new MockCapabilitiesStub()),
          messagingStubAddress(),
          mockParticipantIdStorage(new MockParticipantIdStorage()),
          capabilitiesRegistrar(NULL),
          mockProvider(new MockProvider()),
          domain("testDomain"),
          expectedParticipantId("testParticipantId")
    {

    }
    void SetUp(){
        QList<IDispatcher*> dispatcherList;
        mockDispatcher = new MockDispatcher();
        dispatcherList.append(mockDispatcher);

        capabilitiesRegistrar = new CapabilitiesRegistrar(
                    dispatcherList,
                    mockCapabilitiesStub.dynamicCast<ICapabilities>(),
                    messagingStubAddress,
                    mockParticipantIdStorage
        );
    }
    void TearDown(){
        delete capabilitiesRegistrar;
        delete mockDispatcher;

    }
protected:
    DISALLOW_COPY_AND_ASSIGN(CapabilitiesRegistrarTest);
    MockDispatcher* mockDispatcher;
    QSharedPointer<MockCapabilitiesStub> mockCapabilitiesStub;
    QSharedPointer<EndpointAddressBase> messagingStubAddress;
    QSharedPointer<MockParticipantIdStorage> mockParticipantIdStorage;
    CapabilitiesRegistrar* capabilitiesRegistrar;
    QSharedPointer<MockProvider> mockProvider;
    QString domain;
    QString expectedParticipantId;
};

TEST_F(CapabilitiesRegistrarTest, registerCapability){

    types::ProviderQos testQos;
    testQos.setPriority(100);
    EXPECT_CALL(*mockParticipantIdStorage, getProviderParticipantId(
                    IMockProviderInterface::getInterfaceName(),
                    authToken
    ))
            .Times(1)
            .WillOnce(Return(expectedParticipantId));
    EXPECT_CALL(*mockProvider, getProviderQos())
            .Times(1)
            .WillOnce(Return(testQos));
    EXPECT_CALL(*mockDispatcher, addRequestCaller(expectedParticipantId,_))
            .Times(1);
    EXPECT_CALL(*mockCapabilitiesStub, add(
                    domain,
                    IMockProviderInterface::getInterfaceName(),
                    expectedParticipantId,
                    testQos,
                    _,
                    _,
                    _
    ));

    QString participantId = capabilitiesRegistrar->registerCapability(domain, mockProvider, authToken);
    EXPECT_QSTREQ(expectedParticipantId, participantId);
}

TEST_F(CapabilitiesRegistrarTest, unregisterCapabilityWithDomainAndProviderObject){
    EXPECT_CALL(*mockParticipantIdStorage, getProviderParticipantId(
                    IMockProviderInterface::getInterfaceName(),
                    authToken
    ))
            .Times(1)
            .WillOnce(Return(expectedParticipantId));
    EXPECT_CALL(*mockDispatcher, removeRequestCaller(expectedParticipantId))
            .Times(1);
    EXPECT_CALL(*mockCapabilitiesStub, remove(expectedParticipantId, ICapabilities::NO_TIMEOUT()))
            .Times(1);
    QString participantId = capabilitiesRegistrar->unregisterCapability(domain, mockProvider, authToken);
    EXPECT_QSTREQ(expectedParticipantId, participantId);
}

TEST_F(CapabilitiesRegistrarTest, unregisterCapabilityWithParticipantId){
    EXPECT_CALL(*mockDispatcher, removeRequestCaller(expectedParticipantId))
            .Times(1);
    EXPECT_CALL(*mockCapabilitiesStub, remove(expectedParticipantId, ICapabilities::NO_TIMEOUT()))
            .Times(1);
    capabilitiesRegistrar->unregisterCapability(expectedParticipantId);
}

TEST_F(CapabilitiesRegistrarTest, registerMultipleDispatchersAndRegisterCapability){
    MockDispatcher* mockDispatcher1 = new MockDispatcher();
    MockDispatcher* mockDispatcher2 = new MockDispatcher();
    types::ProviderQos testQos;
    testQos.setPriority(100);

    EXPECT_CALL(*mockParticipantIdStorage, getProviderParticipantId(
                    IMockProviderInterface::getInterfaceName(),
                    authToken
    ))
            .Times(1)
            .WillOnce(Return(expectedParticipantId));
    EXPECT_CALL(*mockProvider, getProviderQos())
            .Times(1)
            .WillOnce(Return(testQos));

    EXPECT_CALL(*mockCapabilitiesStub, add(domain,
                                          IMockProviderInterface::getInterfaceName(),
                                          expectedParticipantId,
                                          testQos,
                                          _,
                                          _,
                                          ICapabilities::NO_TIMEOUT()));

    EXPECT_CALL(*mockDispatcher, addRequestCaller(expectedParticipantId,_))
            .Times(1);
    EXPECT_CALL(*mockDispatcher1, addRequestCaller(expectedParticipantId,_))
            .Times(1);
    EXPECT_CALL(*mockDispatcher2, addRequestCaller(expectedParticipantId,_))
            .Times(1);

    capabilitiesRegistrar->addDispatcher(mockDispatcher1);
    capabilitiesRegistrar->addDispatcher(mockDispatcher2);

    QString participantId = capabilitiesRegistrar->registerCapability(domain, mockProvider, authToken);
    EXPECT_QSTREQ(expectedParticipantId, participantId);

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
                    IMockProviderInterface::getInterfaceName(),
                    authToken
    ))
            .Times(1)
            .WillOnce(Return(expectedParticipantId));
    EXPECT_CALL(*mockProvider, getProviderQos())
            .Times(1)
            .WillOnce(Return(testQos));

    EXPECT_CALL(*mockCapabilitiesStub, add(domain,
                                          IMockProviderInterface::getInterfaceName(),
                                          expectedParticipantId,
                                          testQos,
                                          _,
                                          _,
                                          ICapabilities::NO_TIMEOUT()));

    EXPECT_CALL(*mockDispatcher, addRequestCaller(expectedParticipantId,_))
            .Times(1);
    //mockDispatcher1 should not be used as it was removed
    EXPECT_CALL(*mockDispatcher1, addRequestCaller(expectedParticipantId,_))
            .Times(0);
    EXPECT_CALL(*mockDispatcher2, addRequestCaller(expectedParticipantId,_))
            .Times(1);

    QString participantId = capabilitiesRegistrar->registerCapability(domain, mockProvider, authToken);
    EXPECT_QSTREQ(expectedParticipantId, participantId);

    delete mockDispatcher1;
    delete mockDispatcher2;
}
