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
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <QFile>
#include <string>
#include <stdint.h>
#include "utils/TestQString.h"
#include "joynr/LocalCapabilitiesDirectory.h"
#include "cluster-controller/capabilities-client/ICapabilitiesClient.h"
#include "joynr/ClusterControllerDirectories.h"
#include "joynr/system/ChannelAddress.h"
#include "common/capabilities/CapabilitiesMetaTypes.h"
#include "tests/utils/MockLocalCapabilitiesDirectoryCallback.h"
#include "cluster-controller/capabilities-client/IGlobalCapabilitiesCallback.h"
#include "utils/QThreadSleep.h"
#include "joynr/exceptions.h"
#include "tests/utils/MockObjects.h"
#include "joynr/CapabilityEntry.h"
using namespace ::testing;
using namespace joynr;

class LocalCapabilitiesDirectoryTest : public ::testing::Test {
public:
    LocalCapabilitiesDirectoryTest() :
        settingsFileName("LocalCapabilitiesDirectoryTest.settings"),
        settings(settingsFileName, QSettings::IniFormat),
        messagingSettings(settings),
        capabilitiesClient(new MockCapabilitiesClient()),
        mockMessageRouter(),
        localCapabilitiesDirectory(new LocalCapabilitiesDirectory(messagingSettings, capabilitiesClient, mockMessageRouter)),
        dummyParticipantId1(),
        dummyParticipantId2(),
        localJoynrMessagingAddress1(),
        callback(),
        connections()
    {
        connections.push_back(joynr::types::CommunicationMiddleware::JOYNR);
    }

    ~LocalCapabilitiesDirectoryTest() {
        QFile::remove(settingsFileName);
    }

    void SetUp(){
        registerCapabilitiesMetaTypes();

        //TODO the participantId should be provided by the provider
        dummyParticipantId1 = QUuid::createUuid().toString().toStdString();
        dummyParticipantId2 = QUuid::createUuid().toString().toStdString();
        dummyParticipantId3 = QUuid::createUuid().toString().toStdString();
        localJoynrMessagingAddress1 = QSharedPointer<system::ChannelAddress>(new system::ChannelAddress("LOCAL_CHANNEL_ID"));
        callback = QSharedPointer<MockLocalCapabilitiesDirectoryCallback>(new MockLocalCapabilitiesDirectoryCallback());
        discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL);
        discoveryQos.setCacheMaxAge(10000);
        EXPECT_CALL(*capabilitiesClient, getLocalChannelId()).WillRepeatedly(Return(LOCAL_CHANNEL_ID));

        // init a capentry recieved from the global capabilities directory
        types::ProviderQos qos;
        QList<joynr::types::CommunicationMiddleware::Enum> connections;
        connections.push_back(joynr::types::CommunicationMiddleware::JOYNR);
        CapabilityEntry globalCapEntry(
                    QString::fromStdString(DOMAIN_1_NAME),
                    QString::fromStdString(INTERFACE_1_NAME),
                    qos,
                    QString::fromStdString(dummyParticipantId3),
                    connections,
                    true
        );
        globalCapEntryMap.insert(EXTERNAL_CHANNEL_ID, globalCapEntry);
    }

    void TearDown(){
        delete localCapabilitiesDirectory;
        delete capabilitiesClient;
    }

    void fakeLookupZeroResultsForInterfaceAddress(
            const std::string& domain,
            const std::string& interfaceName,
            std::function<void(
                const RequestStatus& status,
                const std::vector<types::CapabilityInformation>& capability)> callbackFct){
        Q_UNUSED(domain);
        Q_UNUSED(interfaceName);
        RequestStatus status(RequestStatusCode::OK);
        std::vector<types::CapabilityInformation> result;
        callbackFct(status, result);
    }

    void fakeLookupZeroResults(
            const std::string& participantId,
            std::function<void(
                const RequestStatus& status,
                const std::vector<types::CapabilityInformation>& capabilities)> callbackFct){
        Q_UNUSED(participantId);
        RequestStatus status(RequestStatusCode::OK);
        std::vector<types::CapabilityInformation> result;
        callbackFct(status, result);
    }

    void fakeLookupWithResults(
            const std::string& domain,
            const std::string& interfaceName,
            std::function<void(
                const RequestStatus& status,
                const std::vector<types::CapabilityInformation>& capabilities)> callbackFct){
        Q_UNUSED(domain);
        Q_UNUSED(interfaceName);
        types::ProviderQos qos;
        std::vector<types::CapabilityInformation> capInfoList;
        capInfoList.push_back(types::CapabilityInformation(
                               QString::fromStdString(DOMAIN_1_NAME),
                               QString::fromStdString(INTERFACE_1_NAME),
                               qos,
                               QString::fromStdString(EXTERNAL_CHANNEL_ID),
                               QString::fromStdString(dummyParticipantId1)));
        capInfoList.push_back(types::CapabilityInformation(
                               QString::fromStdString(DOMAIN_1_NAME),
                               QString::fromStdString(INTERFACE_1_NAME),
                               qos,
                               QString::fromStdString(LOCAL_CHANNEL_ID),
                               QString::fromStdString(dummyParticipantId2)));
        RequestStatus status(RequestStatusCode::OK);
        callbackFct(status,capInfoList);
    }

    void fakeLookupWithTwoResults(
            const std::string& participantId,
            std::function<void(
                const RequestStatus& status,
                const std::vector<types::CapabilityInformation>& capabilities)> callbackFct){
        types::ProviderQos qos;
        std::vector<types::CapabilityInformation> capInfoList;
        capInfoList.push_back(types::CapabilityInformation(
                               QString::fromStdString(DOMAIN_1_NAME),
                               QString::fromStdString(INTERFACE_1_NAME),
                               qos,
                               QString::fromStdString(LOCAL_CHANNEL_ID),
                               QString::fromStdString(participantId)));
        capInfoList.push_back(types::CapabilityInformation(
                               QString::fromStdString(DOMAIN_2_NAME),
                               QString::fromStdString(INTERFACE_2_NAME),
                               qos,
                               QString::fromStdString(LOCAL_CHANNEL_ID),
                               QString::fromStdString(participantId)));
        RequestStatus status(RequestStatusCode::OK);
        callbackFct(status,capInfoList);
    }

    void fakeLookupWithThreeResults(
            const std::string& participantId,
            std::function<void(
                const RequestStatus& status,
                const std::vector<types::CapabilityInformation>& capabilities)> callbackFct){
        Q_UNUSED(participantId);
        types::ProviderQos qos;
        std::vector<types::CapabilityInformation> capInfoList;
        capInfoList.push_back(types::CapabilityInformation(
                               QString::fromStdString(DOMAIN_1_NAME),
                               QString::fromStdString(INTERFACE_1_NAME),
                               qos,
                               QString::fromStdString(LOCAL_CHANNEL_ID),
                               QString::fromStdString(dummyParticipantId1)));
        capInfoList.push_back(types::CapabilityInformation(
                               QString::fromStdString(DOMAIN_2_NAME),
                               QString::fromStdString(INTERFACE_2_NAME),
                               qos,
                               QString::fromStdString(LOCAL_CHANNEL_ID),
                               QString::fromStdString(dummyParticipantId1)));
        capInfoList.push_back(types::CapabilityInformation(
                               QString::fromStdString(DOMAIN_3_NAME),
                               QString::fromStdString(INTERFACE_3_NAME),
                               qos,
                               QString::fromStdString(EXTERNAL_CHANNEL_ID),
                               QString::fromStdString(dummyParticipantId1)));
        RequestStatus status(RequestStatusCode::OK);
        callbackFct(status,capInfoList);
    }

    void simulateTimeout(){
        throw JoynrTimeOutException("Simulating timeout");
    }

protected:
    QString settingsFileName;
    QSettings settings;
    MessagingSettings messagingSettings;
    MockCapabilitiesClient* capabilitiesClient;
    MockMessageRouter mockMessageRouter;
    LocalCapabilitiesDirectory* localCapabilitiesDirectory;
    std::string dummyParticipantId1;
    std::string dummyParticipantId2;
    std::string dummyParticipantId3;
    QSharedPointer<system::ChannelAddress> localJoynrMessagingAddress1;
    joynr::types::DiscoveryQos discoveryQos;
    QMap<std::string, CapabilityEntry> globalCapEntryMap;

    static const std::string INTERFACE_1_NAME;
    static const std::string DOMAIN_1_NAME;
    static const std::string INTERFACE_2_NAME;
    static const std::string DOMAIN_2_NAME;
    static const std::string INTERFACE_3_NAME;
    static const std::string DOMAIN_3_NAME;
    static const std::string LOCAL_CHANNEL_ID;
    static const std::string EXTERNAL_CHANNEL_ID;
    static const int TIMEOUT;
    QSharedPointer<MockLocalCapabilitiesDirectoryCallback> callback;
    QList<joynr::types::CommunicationMiddleware::Enum> connections;
private:
    DISALLOW_COPY_AND_ASSIGN(LocalCapabilitiesDirectoryTest);
};

const std::string LocalCapabilitiesDirectoryTest::INTERFACE_1_NAME("myInterfaceA");
const std::string LocalCapabilitiesDirectoryTest::INTERFACE_2_NAME("myInterfaceB");
const std::string LocalCapabilitiesDirectoryTest::INTERFACE_3_NAME("myInterfaceC");
const std::string LocalCapabilitiesDirectoryTest::DOMAIN_1_NAME("domainA");
const std::string LocalCapabilitiesDirectoryTest::DOMAIN_2_NAME("domainB");
const std::string LocalCapabilitiesDirectoryTest::DOMAIN_3_NAME("domainB");
const std::string LocalCapabilitiesDirectoryTest::LOCAL_CHANNEL_ID("localChannelId");
const std::string LocalCapabilitiesDirectoryTest::EXTERNAL_CHANNEL_ID("externChannelId");
const int LocalCapabilitiesDirectoryTest::TIMEOUT(2000);



TEST_F(LocalCapabilitiesDirectoryTest, addGloballyDelegatesToCapabilitiesClient) {
    EXPECT_CALL(*capabilitiesClient, add(An<std::vector<types::CapabilityInformation> >())).Times(1);
    joynr::types::DiscoveryEntry entry(
        QString::fromStdString(DOMAIN_1_NAME),
        QString::fromStdString(INTERFACE_1_NAME),
        QString::fromStdString(dummyParticipantId1),
        types::ProviderQos(),
        connections
    );
    localCapabilitiesDirectory->add(entry);
}

TEST_F(LocalCapabilitiesDirectoryTest, addAddsToCache) {
    EXPECT_CALL(*capabilitiesClient, lookup(
                    dummyParticipantId1,
                    A<std::function<void(
                        const RequestStatus& status,
                        const std::vector<joynr::types::CapabilityInformation>& capabilities)>>()))
            .Times(0);
    EXPECT_CALL(*capabilitiesClient, add(_)).Times(1);
    joynr::types::DiscoveryEntry entry(
        QString::fromStdString(DOMAIN_1_NAME),
        QString::fromStdString(INTERFACE_1_NAME),
        QString::fromStdString(dummyParticipantId1),
        types::ProviderQos(),
        connections
    );
    localCapabilitiesDirectory->add(entry);

    localCapabilitiesDirectory->lookup(dummyParticipantId1, callback);
    EXPECT_EQ(1, callback->getResults(TIMEOUT).size());
}

TEST_F(LocalCapabilitiesDirectoryTest, addLocallyDoesNotCallCapabilitiesClient) {
    EXPECT_CALL(*capabilitiesClient, lookup(
                    _,
                    A<std::function<void(
                        const RequestStatus& status,
                        const std::vector<joynr::types::CapabilityInformation>& capabilities)>>()))
            .Times(0);
    EXPECT_CALL(*capabilitiesClient, add(_)).Times(0);
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);
    joynr::types::DiscoveryEntry entry(
        QString::fromStdString(DOMAIN_1_NAME),
        QString::fromStdString(INTERFACE_1_NAME),
        QString::fromStdString(dummyParticipantId1),
        providerQos,
        connections
    );
    localCapabilitiesDirectory->add(entry);

    localCapabilitiesDirectory->lookup(dummyParticipantId1, callback);
    EXPECT_EQ(1, callback->getResults(TIMEOUT).size());

}

TEST_F(LocalCapabilitiesDirectoryTest, removeDelegatesToCapabilitiesClientIfGlobal) {
    EXPECT_CALL(*capabilitiesClient, add(_)).Times(1);
    std::vector<std::string> participantIdsToRemove;
    participantIdsToRemove.push_back(dummyParticipantId1);
    EXPECT_CALL(*capabilitiesClient, remove(participantIdsToRemove)).Times(1);
    joynr::types::DiscoveryEntry entry(
        QString::fromStdString(DOMAIN_1_NAME),
        QString::fromStdString(INTERFACE_1_NAME),
        QString::fromStdString(dummyParticipantId1),
        types::ProviderQos(),
        connections
    );
    localCapabilitiesDirectory->add(entry);
    localCapabilitiesDirectory->remove(DOMAIN_1_NAME ,INTERFACE_1_NAME, types::ProviderQos());
}

TEST_F(LocalCapabilitiesDirectoryTest, removeRemovesFromCache) {
    EXPECT_CALL(*capabilitiesClient, add(_)).Times(1);
    std::vector<std::string> participantIdsToRemove;
    participantIdsToRemove.push_back(dummyParticipantId1);
    EXPECT_CALL(*capabilitiesClient, remove(participantIdsToRemove)).Times(1);
    EXPECT_CALL(*capabilitiesClient, lookup(
                    _,A<std::function<void(
                        const RequestStatus& status,
                        const std::vector<joynr::types::CapabilityInformation>& capabilities)>>()))
            .Times(1)
            .WillOnce(Invoke(this, &LocalCapabilitiesDirectoryTest::fakeLookupZeroResults));

    joynr::types::DiscoveryEntry entry(
        QString::fromStdString(DOMAIN_1_NAME),
        QString::fromStdString(INTERFACE_1_NAME),
        QString::fromStdString(dummyParticipantId1),
        types::ProviderQos(),
        connections
    );
    localCapabilitiesDirectory->add(entry);
    localCapabilitiesDirectory->remove(DOMAIN_1_NAME ,INTERFACE_1_NAME, types::ProviderQos());
    localCapabilitiesDirectory->lookup(dummyParticipantId1, callback);
    EXPECT_EQ(0, callback->getResults(TIMEOUT).size());
}

TEST_F(LocalCapabilitiesDirectoryTest, removeLocalCapabilityByInterfaceAddressDoesNotDelegateToBackEnd) {
    EXPECT_CALL(*capabilitiesClient, add(_)).Times(0);
    EXPECT_CALL(*capabilitiesClient, remove(dummyParticipantId1)).Times(0);

    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);
    joynr::types::DiscoveryEntry entry(
        QString::fromStdString(DOMAIN_1_NAME),
        QString::fromStdString(INTERFACE_1_NAME),
        QString::fromStdString(dummyParticipantId1),
        providerQos,
        connections
    );
    localCapabilitiesDirectory->add(entry);
    localCapabilitiesDirectory->remove(DOMAIN_1_NAME ,INTERFACE_1_NAME, providerQos);
}



TEST_F(LocalCapabilitiesDirectoryTest, lookupForInterfaceAddressReturnsCachedValues) {

    //simulate global capability directory would store two entries.
    EXPECT_CALL(*capabilitiesClient, lookup(DOMAIN_1_NAME ,INTERFACE_1_NAME,_))
            .Times(1)
            .WillOnce(Invoke(this, &LocalCapabilitiesDirectoryTest::fakeLookupWithResults));

    localCapabilitiesDirectory->lookup(DOMAIN_1_NAME ,INTERFACE_1_NAME, callback, discoveryQos);
    callback->clearResults();
    //enries are now in cache, capabilitiesClient should not be called.
    EXPECT_CALL(*capabilitiesClient, lookup(
                    _,
                    _,
                    A<std::function<void(
                        const RequestStatus& status,
                        const std::vector<joynr::types::CapabilityInformation>& capabilities)>>()))
            .Times(0);
    localCapabilitiesDirectory->lookup(DOMAIN_1_NAME ,INTERFACE_1_NAME, callback, discoveryQos);
    EXPECT_EQ(2, callback->getResults(TIMEOUT).size());
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupForInterfaceAddressDelegatesToCapabilitiesClient) {
    //simulate global capability directory would store two entries.
    EXPECT_CALL(*capabilitiesClient, lookup(
                    DOMAIN_1_NAME ,
                    INTERFACE_1_NAME,
                    _))
            .Times(1)
            .WillOnce(Invoke(this, &LocalCapabilitiesDirectoryTest::fakeLookupWithResults));

    localCapabilitiesDirectory->lookup(DOMAIN_1_NAME ,INTERFACE_1_NAME, callback, discoveryQos);
    std::vector<CapabilityEntry> capabilities = callback->getResults(TIMEOUT);


    EXPECT_EQ(2, capabilities.size());

    // check that the results contain the two channel ids
    bool firstParticipantIdFound = false;
    bool secondParticipantIdFound = false;
    for (uint16_t i = 0; i < capabilities.size(); i++) {
        CapabilityEntry entry = capabilities.at(i);
        EXPECT_EQ(DOMAIN_1_NAME, entry.getDomain().toStdString());
        EXPECT_EQ(INTERFACE_1_NAME, entry.getInterfaceName().toStdString());
        std::string participantId = entry.getParticipantId().toStdString();
        if (participantId == dummyParticipantId1) {
            firstParticipantIdFound = true;
        } else if (participantId == dummyParticipantId2) {
            secondParticipantIdFound = true;
        }
    }

    EXPECT_TRUE(firstParticipantIdFound);
    EXPECT_TRUE(secondParticipantIdFound);
}

TEST_F(LocalCapabilitiesDirectoryTest, lookupForParticipantIdReturnsCachedValues) {

    EXPECT_CALL(*capabilitiesClient, lookup(
                    _,
                    A<std::function<void(
                        const RequestStatus& status,
                        const std::vector<types::CapabilityInformation>& capabilities)>>()))
            .Times(1)
            .WillOnce(Invoke(this, &LocalCapabilitiesDirectoryTest::fakeLookupWithTwoResults));

    localCapabilitiesDirectory->lookup(dummyParticipantId1, callback);
    callback->clearResults();
    EXPECT_CALL(*capabilitiesClient, lookup(
                    _,
                    A<std::function<void(
                        const RequestStatus& status,
                        const std::vector<types::CapabilityInformation>& capabilities)>>()))
            .Times(0);
    localCapabilitiesDirectory->lookup(dummyParticipantId1, callback);
    std::vector<CapabilityEntry> capabilities = callback->getResults(TIMEOUT);
    EXPECT_EQ(2, capabilities.size());

}

TEST_F(LocalCapabilitiesDirectoryTest, lookupForParticipantIdDelegatesToCapabilitiesClient) {

    EXPECT_CALL(*capabilitiesClient, lookup(
                    dummyParticipantId1,
                    A<std::function<void(
                        const RequestStatus& status,
                        const std::vector<types::CapabilityInformation>& capabilities)>>()))
            .Times(1)
            .WillOnce(Invoke(this, &LocalCapabilitiesDirectoryTest::fakeLookupWithThreeResults));

    localCapabilitiesDirectory->lookup(dummyParticipantId1, callback);
    std::vector<CapabilityEntry> capabilities = callback->getResults(TIMEOUT);

    EXPECT_EQ(3, capabilities.size());
    bool interfaceAddress1Found = false;
    bool interfaceAddress2Found = false;
    for (uint16_t i = 0; i < capabilities.size(); i++) {
        CapabilityEntry entry = capabilities.at(i);
        if ((entry.getDomain().toStdString() == DOMAIN_1_NAME) && (entry.getInterfaceName().toStdString() == INTERFACE_1_NAME)) {
            interfaceAddress1Found = true;
        } else if ((entry.getDomain().toStdString() == DOMAIN_2_NAME) && (entry.getInterfaceName().toStdString() == INTERFACE_2_NAME)) {
            interfaceAddress2Found = true;
        }
    }

    EXPECT_TRUE(interfaceAddress1Found);
    EXPECT_TRUE(interfaceAddress2Found);

}

TEST_F(LocalCapabilitiesDirectoryTest, cleanCacheRemovesOldEntries) {

    EXPECT_CALL(*capabilitiesClient, lookup(
                    _,
                    A<std::function<void(
                        const RequestStatus& status,
                        const std::vector<types::CapabilityInformation>& capabilities)>>()))
            .Times(1)
            .WillOnce(Invoke(this, &LocalCapabilitiesDirectoryTest::fakeLookupWithTwoResults));

    localCapabilitiesDirectory->lookup(dummyParticipantId1, callback);
    QThreadSleep::msleep(1000);

    // this should remove all entries in the cache
    localCapabilitiesDirectory->cleanCache(100);
    // retrieving capabilities will force a call to the backend as the cache is empty
    EXPECT_CALL(*capabilitiesClient, lookup(
                    _,
                    A<std::function<void(
                        const RequestStatus& status,
                        const std::vector<types::CapabilityInformation>& capabilities)>>()))
            .Times(1);
    localCapabilitiesDirectory->lookup(dummyParticipantId1, callback);

}

TEST_F(LocalCapabilitiesDirectoryTest, registerMultipleGlobalCapabilitiesCheckIfTheyAreMerged) {

    std::vector<types::CapabilityInformation> firstCapInfoList;
    std::vector<types::CapabilityInformation> secondCapInfoList;
    types::ProviderQos qos;
    types::CapabilityInformation capInfo1(types::CapabilityInformation(
                                              QString::fromStdString(DOMAIN_1_NAME),
                                              QString::fromStdString(INTERFACE_1_NAME),
                                              qos,
                                              QString::fromStdString(LOCAL_CHANNEL_ID),
                                              QString::fromStdString(dummyParticipantId1)));
    firstCapInfoList.push_back(capInfo1);
    secondCapInfoList.push_back(capInfo1);
    secondCapInfoList.push_back(types::CapabilityInformation(
                                 QString::fromStdString(DOMAIN_2_NAME),
                                 QString::fromStdString(INTERFACE_1_NAME),
                                 qos,
                                 QString::fromStdString(LOCAL_CHANNEL_ID),
                                 QString::fromStdString(dummyParticipantId2)));

    {
        InSequence inSequence;
        EXPECT_CALL(*capabilitiesClient, add(firstCapInfoList)).Times(1);
        EXPECT_CALL(*capabilitiesClient, add(secondCapInfoList)).Times(1);
    }


    joynr::types::DiscoveryEntry entry(
        QString::fromStdString(DOMAIN_1_NAME),
        QString::fromStdString(INTERFACE_1_NAME),
        QString::fromStdString(dummyParticipantId1),
        qos,
        connections
    );
    localCapabilitiesDirectory->add(entry);
    joynr::types::DiscoveryEntry entry2(
        QString::fromStdString(DOMAIN_2_NAME),
        QString::fromStdString(INTERFACE_1_NAME),
        QString::fromStdString(dummyParticipantId2),
        qos,
        connections
    );
    localCapabilitiesDirectory->add(entry2);
}

TEST_F(LocalCapabilitiesDirectoryTest, testRegisterCapabilitiesMultipleTimesDoesNotDuplicate) {
    types::ProviderQos qos;
    int exceptionCounter = 0;
    //simulate capabilities client lost connection to the directory
    EXPECT_CALL(*capabilitiesClient, add(_)).Times(3).WillRepeatedly(InvokeWithoutArgs(this, &LocalCapabilitiesDirectoryTest::simulateTimeout));

    for (int i = 0; i<3; i++){
        try {
            joynr::types::DiscoveryEntry entry(
                QString::fromStdString(DOMAIN_1_NAME),
                QString::fromStdString(INTERFACE_1_NAME),
                QString::fromStdString(dummyParticipantId1),
                qos,
                connections
            );
            localCapabilitiesDirectory->add(entry);
        } catch (JoynrException& e){
            exceptionCounter++;
        }
    }
    EXPECT_EQ(3, exceptionCounter);
    localCapabilitiesDirectory->lookup(DOMAIN_1_NAME,INTERFACE_1_NAME,callback, discoveryQos);
    std::vector<CapabilityEntry> capabilities = callback->getResults(100);
    EXPECT_EQ(1, capabilities.size());
}

TEST_F(LocalCapabilitiesDirectoryTest, removeLocalCapabilityByParticipantId){
    types::ProviderQos qos;
    EXPECT_CALL(*capabilitiesClient, lookup(
                    _,
                    A<std::function<void(
                        const RequestStatus& status,
                        const std::vector<types::CapabilityInformation>& capabilities)>>()))
            .Times(0);

    joynr::types::DiscoveryEntry entry(
        QString::fromStdString(DOMAIN_1_NAME),
        QString::fromStdString(INTERFACE_1_NAME),
        QString::fromStdString(dummyParticipantId1),
        qos,
        connections
    );
    localCapabilitiesDirectory->add(entry);
    localCapabilitiesDirectory->lookup(dummyParticipantId1, callback);
    EXPECT_EQ(1, callback->getResults(10).size());
    callback->clearResults();

    localCapabilitiesDirectory->remove(dummyParticipantId1);

    EXPECT_CALL(*capabilitiesClient, lookup(
                    _,
                    A<std::function<void(
                        const RequestStatus& status,
                        const std::vector<types::CapabilityInformation>& capabilities)>>()))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(this, &LocalCapabilitiesDirectoryTest::simulateTimeout));
    //JoynrTimeOutException timeoutException;
    EXPECT_THROW(localCapabilitiesDirectory->lookup(dummyParticipantId1, callback),
                 JoynrTimeOutException);
    EXPECT_EQ(0, callback->getResults(10).size());
}

TEST_F(LocalCapabilitiesDirectoryTest, registerLocalCapability_lookupLocal){
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_ONLY);

    EXPECT_CALL(*capabilitiesClient, add(_)).Times(0);
    joynr::types::DiscoveryEntry entry(
        QString::fromStdString(DOMAIN_1_NAME),
        QString::fromStdString(INTERFACE_1_NAME),
        QString::fromStdString(dummyParticipantId1),
        providerQos,
        connections
    );
    localCapabilitiesDirectory->add(entry);
    localCapabilitiesDirectory->registerReceivedCapabilities(globalCapEntryMap);

    EXPECT_CALL(*capabilitiesClient, lookup(_,_,_)).Times(0);
    localCapabilitiesDirectory->lookup(DOMAIN_1_NAME, INTERFACE_1_NAME, callback, discoveryQos);
    EXPECT_EQ(1, callback->getResults(10).size());
    callback->clearResults();

    EXPECT_CALL(*capabilitiesClient, remove(dummyParticipantId1)).Times(1);
    localCapabilitiesDirectory->remove(dummyParticipantId1);

    localCapabilitiesDirectory->lookup(DOMAIN_1_NAME, INTERFACE_1_NAME, callback, discoveryQos);
    EXPECT_EQ(0, callback->getResults(10).size());
    callback->clearResults();
}

TEST_F(LocalCapabilitiesDirectoryTest, registerLocalCapability_lookupLocalThenGlobal){
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL);

    joynr::types::DiscoveryEntry entry(
        QString::fromStdString(DOMAIN_1_NAME),
        QString::fromStdString(INTERFACE_1_NAME),
        QString::fromStdString(dummyParticipantId1),
        providerQos,
        connections
    );
    EXPECT_CALL(*capabilitiesClient, add(_)).Times(0);
    localCapabilitiesDirectory->add(entry);
    localCapabilitiesDirectory->registerReceivedCapabilities(globalCapEntryMap);

    EXPECT_CALL(*capabilitiesClient, lookup(_,_,_)).Times(0);
    localCapabilitiesDirectory->lookup(DOMAIN_1_NAME, INTERFACE_1_NAME, callback, discoveryQos);
    EXPECT_EQ(1, callback->getResults(10).size());
    callback->clearResults();

    EXPECT_CALL(*capabilitiesClient, remove(dummyParticipantId1)).Times(1);
    localCapabilitiesDirectory->remove(dummyParticipantId1);

    localCapabilitiesDirectory->lookup(DOMAIN_1_NAME, INTERFACE_1_NAME, callback, discoveryQos);
    EXPECT_EQ(1, callback->getResults(10).size());
    callback->clearResults();

    // disable cache
    discoveryQos.setCacheMaxAge(0);
    EXPECT_CALL(*capabilitiesClient, lookup(_,_,_))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(this, &LocalCapabilitiesDirectoryTest::simulateTimeout));
    EXPECT_THROW(localCapabilitiesDirectory->lookup(DOMAIN_1_NAME, INTERFACE_1_NAME, callback, discoveryQos),
                 JoynrTimeOutException);
    EXPECT_EQ(0, callback->getResults(10).size());
    callback->clearResults();
}

TEST_F(LocalCapabilitiesDirectoryTest, registerLocalCapability_lookupLocalAndGlobal){
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_AND_GLOBAL);

    joynr::types::DiscoveryEntry entry(
        QString::fromStdString(DOMAIN_1_NAME),
        QString::fromStdString(INTERFACE_1_NAME),
        QString::fromStdString(dummyParticipantId1),
        providerQos,
        connections
    );
    EXPECT_CALL(*capabilitiesClient, add(_)).Times(0);
    localCapabilitiesDirectory->add(entry);
    //localCapabilitiesDirectory->registerReceivedCapabilities(globalCapEntryMap);

    EXPECT_CALL(*capabilitiesClient, lookup(_,_,_))
            .Times(2)
//            .WillOnce(InvokeWithoutArgs(this, &LocalCapabilitiesDirectoryTest::simulateTimeout));
            .WillRepeatedly(Invoke(this, &LocalCapabilitiesDirectoryTest::fakeLookupZeroResultsForInterfaceAddress));
    localCapabilitiesDirectory->lookup(DOMAIN_1_NAME, INTERFACE_1_NAME, callback, discoveryQos);
    EXPECT_EQ(1, callback->getResults(10).size());
    callback->clearResults();

    EXPECT_CALL(*capabilitiesClient, remove(dummyParticipantId1)).Times(1);
    localCapabilitiesDirectory->remove(dummyParticipantId1);

    localCapabilitiesDirectory->lookup(DOMAIN_1_NAME, INTERFACE_1_NAME, callback, discoveryQos);
    EXPECT_EQ(0, callback->getResults(10).size());
    callback->clearResults();

    // disable cache
    discoveryQos.setCacheMaxAge(0);
    EXPECT_CALL(*capabilitiesClient, lookup(_,_,_))
            .Times(1)
            .WillOnce(Invoke(this, &LocalCapabilitiesDirectoryTest::fakeLookupWithResults));
    localCapabilitiesDirectory->lookup(DOMAIN_1_NAME, INTERFACE_1_NAME, callback, discoveryQos);
    EXPECT_EQ(2, callback->getResults(10).size());
    callback->clearResults();

    EXPECT_CALL(*capabilitiesClient, lookup(_,_,_))
            .Times(0);
    localCapabilitiesDirectory->cleanCache(-1);

    discoveryQos.setCacheMaxAge(4000);
    localCapabilitiesDirectory->registerReceivedCapabilities(globalCapEntryMap);
    localCapabilitiesDirectory->lookup(DOMAIN_1_NAME, INTERFACE_1_NAME, callback, discoveryQos);
    EXPECT_EQ(1, callback->getResults(10).size());
}

TEST_F(LocalCapabilitiesDirectoryTest, registerLocalCapability_lookupGlobalOnly){
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::LOCAL);

    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::GLOBAL_ONLY);

    EXPECT_CALL(*capabilitiesClient, add(_)).Times(0);
    joynr::types::DiscoveryEntry entry(
        QString::fromStdString(DOMAIN_1_NAME),
        QString::fromStdString(INTERFACE_1_NAME),
        QString::fromStdString(dummyParticipantId1),
        providerQos,
        connections
    );
    localCapabilitiesDirectory->add(entry);

    EXPECT_CALL(*capabilitiesClient, lookup(_,_,_))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(this, &LocalCapabilitiesDirectoryTest::simulateTimeout));
    //JoynrTimeOutException timeoutException;
    EXPECT_THROW(localCapabilitiesDirectory->lookup(DOMAIN_1_NAME, INTERFACE_1_NAME, callback, discoveryQos),
                 JoynrTimeOutException);
    EXPECT_EQ(0, callback->getResults(10).size());
    callback->clearResults();

    // register the external capability
    localCapabilitiesDirectory->registerReceivedCapabilities(globalCapEntryMap);
    // get the global entry
    EXPECT_CALL(*capabilitiesClient, lookup(_,_,_)).Times(0);
    localCapabilitiesDirectory->lookup(DOMAIN_1_NAME, INTERFACE_1_NAME, callback, discoveryQos);
    EXPECT_EQ(1, callback->getResults(10).size());
    callback->clearResults();

    EXPECT_CALL(*capabilitiesClient, remove(dummyParticipantId1)).Times(1);
    localCapabilitiesDirectory->remove(dummyParticipantId1);

    // disable cache
    discoveryQos.setCacheMaxAge(0);
    EXPECT_CALL(*capabilitiesClient, lookup(_,_,_))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(this, &LocalCapabilitiesDirectoryTest::simulateTimeout));
    EXPECT_THROW(localCapabilitiesDirectory->lookup(DOMAIN_1_NAME, INTERFACE_1_NAME, callback, discoveryQos),
                 JoynrTimeOutException);
    EXPECT_EQ(0, callback->getResults(10).size());
    callback->clearResults();
}

TEST_F(LocalCapabilitiesDirectoryTest, registerGlobalCapability_lookupLocal){
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);

    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(5000);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_ONLY);

    EXPECT_CALL(*capabilitiesClient, add(_)).Times(1);
    joynr::types::DiscoveryEntry entry(
        QString::fromStdString(DOMAIN_1_NAME),
        QString::fromStdString(INTERFACE_1_NAME),
        QString::fromStdString(dummyParticipantId1),
        providerQos,
        connections
    );
    localCapabilitiesDirectory->add(entry);
    localCapabilitiesDirectory->registerReceivedCapabilities(globalCapEntryMap);

    EXPECT_CALL(*capabilitiesClient, lookup(_,_,_)).Times(0);
    localCapabilitiesDirectory->lookup(DOMAIN_1_NAME, INTERFACE_1_NAME, callback, discoveryQos);
    EXPECT_EQ(1, callback->getResults(10).size());
    callback->clearResults();

    EXPECT_CALL(*capabilitiesClient, remove(dummyParticipantId1)).Times(1);
    localCapabilitiesDirectory->remove(dummyParticipantId1);
}

TEST_F(LocalCapabilitiesDirectoryTest, registerGlobalCapability_lookupLocalThenGlobal){
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);

    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(100);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL);

    EXPECT_CALL(*capabilitiesClient, add(_)).Times(1);
    joynr::types::DiscoveryEntry entry(
        QString::fromStdString(DOMAIN_1_NAME),
        QString::fromStdString(INTERFACE_1_NAME),
        QString::fromStdString(dummyParticipantId1),
        providerQos,
        connections
    );
    localCapabilitiesDirectory->add(entry);
    localCapabilitiesDirectory->registerReceivedCapabilities(globalCapEntryMap);

    // get the local entry
    EXPECT_CALL(*capabilitiesClient, lookup(_,_,_)).Times(0);
    localCapabilitiesDirectory->lookup(DOMAIN_1_NAME, INTERFACE_1_NAME, callback, discoveryQos);
    EXPECT_EQ(1, callback->getResults(10).size());
    callback->clearResults();

    EXPECT_CALL(*capabilitiesClient, remove(dummyParticipantId1)).Times(1);
    localCapabilitiesDirectory->remove(dummyParticipantId1);

    // get the global entry
    EXPECT_CALL(*capabilitiesClient, lookup(_,_,_)).Times(0);
    localCapabilitiesDirectory->lookup(DOMAIN_1_NAME, INTERFACE_1_NAME, callback, discoveryQos);
    EXPECT_EQ(1, callback->getResults(10).size());
    callback->clearResults();

    QThreadSleep::msleep(200);

    // get the global, but timeout occured
    EXPECT_CALL(*capabilitiesClient, lookup(_,_,_))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(this, &LocalCapabilitiesDirectoryTest::simulateTimeout));
    EXPECT_THROW(localCapabilitiesDirectory->lookup(DOMAIN_1_NAME, INTERFACE_1_NAME, callback, discoveryQos),
                 JoynrTimeOutException);

    EXPECT_EQ(0, callback->getResults(10).size());
}

TEST_F(LocalCapabilitiesDirectoryTest, registerGlobalCapability_lookupGlobalOnly){
    types::ProviderQos providerQos;
    providerQos.setScope(types::ProviderScope::GLOBAL);

    joynr::types::DiscoveryQos discoveryQos;
    discoveryQos.setCacheMaxAge(100);
    discoveryQos.setDiscoveryScope(joynr::types::DiscoveryScope::GLOBAL_ONLY);

    //JoynrTimeOutException timeoutException;
    EXPECT_CALL(*capabilitiesClient, add(_)).Times(1);
    joynr::types::DiscoveryEntry entry(
        QString::fromStdString(DOMAIN_1_NAME),
        QString::fromStdString(INTERFACE_1_NAME),
        QString::fromStdString(dummyParticipantId1),
        providerQos,
        connections
    );
    localCapabilitiesDirectory->add(entry);

    EXPECT_CALL(*capabilitiesClient, lookup(_,_,_)).Times(0);
    localCapabilitiesDirectory->lookup(DOMAIN_1_NAME, INTERFACE_1_NAME, callback, discoveryQos);
    EXPECT_EQ(1, callback->getResults(10).size());
    callback->clearResults();

    //recieve a global entry
    localCapabilitiesDirectory->registerReceivedCapabilities(globalCapEntryMap);
    EXPECT_CALL(*capabilitiesClient, lookup(_,_,_)).Times(0);
    localCapabilitiesDirectory->lookup(DOMAIN_1_NAME, INTERFACE_1_NAME, callback, discoveryQos);
    EXPECT_EQ(2, callback->getResults(10).size());

    EXPECT_CALL(*capabilitiesClient, remove(dummyParticipantId1)).Times(1);
    localCapabilitiesDirectory->remove(dummyParticipantId1);
}

//TODO test remove global capability
