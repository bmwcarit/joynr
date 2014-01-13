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
#include <gtest/gtest.h>
#include "joynr/MessagingSettings.h"
#include "tests/utils/MockObjects.h"
#include "joynr/IDbusSkeletonWrapper.h"
#include "common/dbus/DbusMessagingStubAdapter.h"
#include "joynr/DbusMessagingSkeleton.h"
#include "joynr/DbusCapabilitiesSkeleton.h"
#include "libjoynr/dbus/DbusCapabilitiesStubAdapter.h"
#include "joynr/JoynrMessagingEndpointAddress.h"
#include "common/dbus/DbusMessagingEndpointAddress.h"
#include "QString"

#include "tests/utils/MockObjects.h"
#include "joynr/IMessaging.h"

#include <thread>
#include <chrono>

using namespace joynr;

class LibJoynrDbusCommunicationTests : public testing::Test {

public:

    QString settingsFilename;
    QSettings settings;
    MessagingSettings* messagingSettings;
    Logger* logger;

    LibJoynrDbusCommunicationTests():
        settingsFilename("resources/integrationtest.settings"),
        settings(settingsFilename, QSettings::IniFormat),
        messagingSettings(new MessagingSettings(settings))
    {
        logger = Logging::getInstance()->getLogger("TST", "LibJoynrDbusCommunicationTests");
    }

    ~LibJoynrDbusCommunicationTests() {
    }

    void SetUp() {
    }

    void printResult(Logger* logger, QString text, bool result) {
        if(result) LOG_INFO(logger, text + " SUCCESS"); else LOG_ERROR(logger, text + " ERROR");
    }
};

TEST_F(LibJoynrDbusCommunicationTests, dbus_commonapi_runtime_feature_check) {
    QString ccMessagingAddress("local:cc.messaging:cc.messaging8");

    // create the skeleton
    MockMessaging* msgMock = new MockMessaging();
    EXPECT_CALL(*msgMock, transmit(A<JoynrMessage&>(), A<const MessagingQos&>())).Times(2);
    auto provider = std::make_shared<DbusMessagingSkeleton>(*msgMock);

    // register skeleton
    auto runtime = CommonAPI::Runtime::load("DBus");
    auto factory = runtime->createFactory();

    bool success = runtime->getServicePublisher()->registerService(provider, ccMessagingAddress.toStdString(), factory);
    ASSERT_TRUE(success);
    printResult(logger, "registerService", success);

    // get proxy
    auto runtime2 = CommonAPI::Runtime::load("DBus");
    auto factory2 = runtime2->createFactory();
    auto proxy = factory2->buildProxy<joynr::messaging::IMessagingProxy>(ccMessagingAddress.toStdString());
    // wait some time so that the proxy is ready to use on dbus level
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // call method
    joynr::messaging::IMessaging::JoynrMessage message;
    message.type =  "reply";
    message.payload = "This is a test";
    joynr::messaging::types::Types::JoynrMessageQos qos;
    CommonAPI::CallStatus status;
    ASSERT_TRUE(proxy->isAvailable());
    proxy->transmit(message, qos, status);
    printResult(logger, "transmit status", status == CommonAPI::CallStatus::SUCCESS);
    ASSERT_TRUE(status == CommonAPI::CallStatus::SUCCESS);

    // unregister service
    success = runtime->getServicePublisher()->unregisterService(ccMessagingAddress.toStdString());
    printResult(logger, "unregisterService", success);
    ASSERT_TRUE(success);

    // call method
    proxy->transmit(message, qos, status);
    printResult(logger, "transmit status", status == CommonAPI::CallStatus::SUCCESS);
    ASSERT_FALSE(status == CommonAPI::CallStatus::SUCCESS);

    printResult(logger, "proxy available:", proxy->isAvailable());

    // register service
    success = runtime->getServicePublisher()->registerService(provider, ccMessagingAddress.toStdString(), factory);
    printResult(logger, "registerService", success);
    ASSERT_TRUE(success);
    // wait some time so that the service is registered on dbus level
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // call method
    proxy->transmit(message, qos, status);
    printResult(logger, "transmit status", status == CommonAPI::CallStatus::SUCCESS);
    ASSERT_TRUE(status == CommonAPI::CallStatus::SUCCESS);

    // unregister service
    success = runtime->getServicePublisher()->unregisterService(ccMessagingAddress.toStdString());
    printResult(logger, "unregisterService", success);
    ASSERT_TRUE(success);

    delete msgMock;
}

TEST_F(LibJoynrDbusCommunicationTests, dbus_skeletonwrapper_register_unregister) {
    QString ccMessagingAddress("local:cc.messaging:cc.messaging8");

    // craete mock and expect 2 calls
    MockMessaging* msgMock = new MockMessaging();
    EXPECT_CALL(*msgMock, transmit(A<JoynrMessage&>(), A<const MessagingQos&>())).Times(2);

    // create the skeleton
    LOG_INFO(logger, "Register skeleton");
    auto msgSkeleton = new IDbusSkeletonWrapper<DbusMessagingSkeleton, IMessaging>(*msgMock, ccMessagingAddress);

    // create message
    JoynrMessage msg;
    msg.setType(JoynrMessage::VALUE_MESSAGE_TYPE_ONE_WAY);
    msg.setHeaderTo(QString("local"));
    msg.setPayload("This is a test");
    MessagingQos qos;

    // get stub
    DbusMessagingStubAdapter* msgStub = new DbusMessagingStubAdapter(ccMessagingAddress);
    ASSERT_TRUE(msgStub->isProxyAvailabe());

    // call method
    LOG_INFO(logger, "Transmit message: should work");
    msgStub->transmit(msg, qos);

    // delete skeleton
    LOG_INFO(logger, "Delete skeleton");
    delete msgSkeleton;

    // call method
    LOG_INFO(logger, "Transmit message: should fail");
    msgStub->transmit(msg, qos);

    // register skeleton
    LOG_INFO(logger, "Register skeleton");
    msgSkeleton = new IDbusSkeletonWrapper<DbusMessagingSkeleton, IMessaging>(*msgMock, ccMessagingAddress);

    // call method
    LOG_INFO(logger, "Transmit message: should work");
    msgStub->transmit(msg, qos);

    delete msgSkeleton;

    delete msgMock;
}

TEST_F(LibJoynrDbusCommunicationTests, DISABLED_connection_test) {
    QString ccMessagingAddress("local:cc.messaging:cc.messaging7");

    // register skeletons
    MockMessaging* msgMock = new MockMessaging();
    auto msgSkeleton = new IDbusSkeletonWrapper<DbusMessagingSkeleton, IMessaging>(*msgMock, ccMessagingAddress);

    // get stub
    DbusMessagingStubAdapter* msgStub = new DbusMessagingStubAdapter(ccMessagingAddress);
    ASSERT_TRUE(msgStub->isProxyAvailabe());

    // delete skeleton
    delete msgSkeleton;

    ASSERT_FALSE(msgStub->isProxyAvailabe());
    delete msgStub;
}

TEST_F(LibJoynrDbusCommunicationTests, transmit_message) {
    QString ccMessagingAddress("local:joynr.messaging:cc.message6");

    // register skeletons
    MockMessaging* msgMock = new MockMessaging();
    EXPECT_CALL(*msgMock, transmit(A<JoynrMessage&>(), A<const MessagingQos&>())).Times(1);
    auto msgSkeleton = new IDbusSkeletonWrapper<DbusMessagingSkeleton, IMessaging>(*msgMock, ccMessagingAddress);

    // get stub
    DbusMessagingStubAdapter* msgStub = new DbusMessagingStubAdapter(ccMessagingAddress);
    ASSERT_TRUE(msgStub->isProxyAvailabe());

    // create message
    JoynrMessage msg;
    msg.setType(JoynrMessage::VALUE_MESSAGE_TYPE_ONE_WAY);
    msg.setHeaderTo(QString("local"));
    msg.setPayload("This is a test");

    // create messaging qos
    MessagingQos qos;
    msgStub->transmit(msg, qos);

    // delete skeleton
    delete msgSkeleton;

    // error on transmission
    msgStub->transmit(msg, qos);

    // stub not availabe
    ASSERT_FALSE(msgStub->isProxyAvailabe());

    delete msgStub;
    delete msgMock;
}

TEST_F(LibJoynrDbusCommunicationTests, capabilities_call_lookup1) {
    QString ccCapabilitiesAddress("local:joynr.capabilities:cc.message5");

    // register skeletons
    MockCapabilitiesStub* capaMock = new MockCapabilitiesStub();

    // create default entry
    QList<CapabilityEntry>* result = new QList<CapabilityEntry>();
    types::ProviderQos* pQos = new types::ProviderQos();
    pQos->setPriority(5);
    QSharedPointer<JoynrMessagingEndpointAddress> endPoint(new JoynrMessagingEndpointAddress("defaultChannelId"));
    CapabilityEntry* defaultEntry = new CapabilityEntry();
    defaultEntry->setParticipantId("defaultDbusId");
    defaultEntry->setQos(*pQos);
    defaultEntry->prependEndpointAddress(endPoint);
    result->append(*defaultEntry);

    EXPECT_CALL(*capaMock, lookup(A<const QString&>(),
                                  A<const QString&>(),
                                  A<const types::ProviderQosRequirements&>(),
                                  A<const DiscoveryQos&>())).Times(1).WillRepeatedly(testing::Return(*result));
    auto capaSkeleton = new IDbusSkeletonWrapper<DbusCapabilitiesSkeleton, ICapabilities>(*capaMock, ccCapabilitiesAddress);

    // get stub
    DbusCapabilitiesStubAdapter* capaStub = new DbusCapabilitiesStubAdapter(ccCapabilitiesAddress);
    ASSERT_TRUE(capaStub->isProxyAvailabe());

    // call methodl
    QList<CapabilityEntry> methodResult = capaStub->lookup(QString("local"), QString("myinterface"), types::ProviderQosRequirements(true), DiscoveryQos(1000));
    ASSERT_TRUE(methodResult.size() == result->size());

    // check for the same capabilities entry
    ASSERT_TRUE(methodResult.at(0) == (*defaultEntry));

    // delete skeleton
    delete capaSkeleton;
    delete capaStub;
    delete capaMock;
}

TEST_F(LibJoynrDbusCommunicationTests, capabilities_call_lookup2) {
    QString ccCapabilitiesAddress("local:joynr.capabilities:cc.message4");

    // register skeletons
    MockCapabilitiesStub* capaMock = new MockCapabilitiesStub();

    // create default entry
    QList<CapabilityEntry>* result = new QList<CapabilityEntry>();
    types::ProviderQos* pQos = new types::ProviderQos();
    pQos->setPriority(5);
    QSharedPointer<JoynrMessagingEndpointAddress> endPoint(new JoynrMessagingEndpointAddress("defaultChannelId"));
    CapabilityEntry* defaultEntry = new CapabilityEntry();
    defaultEntry->setParticipantId("defaultDbusId");
    defaultEntry->setQos(*pQos);
    defaultEntry->prependEndpointAddress(endPoint);
    result->append(*defaultEntry);

    EXPECT_CALL(*capaMock, lookup(A<const QString&>(),
                                  A<const DiscoveryQos&>())).Times(1).WillRepeatedly(testing::Return(*result));
    auto capaSkeleton = new IDbusSkeletonWrapper<DbusCapabilitiesSkeleton, ICapabilities>(*capaMock, ccCapabilitiesAddress);

    // get stub
    DbusCapabilitiesStubAdapter* capaStub = new DbusCapabilitiesStubAdapter(ccCapabilitiesAddress);
    ASSERT_TRUE(capaStub->isProxyAvailabe());

    // call methodl
    QList<CapabilityEntry> methodResult = capaStub->lookup(QString("local"), DiscoveryQos(1000));
    ASSERT_TRUE(methodResult.size() == result->size());

    // check for the same capabilities entry
    ASSERT_TRUE(methodResult.at(0) == (*defaultEntry));

    // delete skeleton
    delete capaSkeleton;
    delete capaStub;
    delete capaMock;
}


TEST_F(LibJoynrDbusCommunicationTests, capabilities_call_addEndPoint) {
    QString ccCapabilitiesAddress("local:joynr.capabilities:cc.message3");

    // register skeletons
    MockCapabilitiesStub* capaMock = new MockCapabilitiesStub();

    EXPECT_CALL(*capaMock, addEndpoint(A<const QString&>(),
                                  A<QSharedPointer<EndpointAddressBase>>(),
                                  A<const qint64&>())).Times(1);
    auto capaSkeleton = new IDbusSkeletonWrapper<DbusCapabilitiesSkeleton, ICapabilities>(*capaMock, ccCapabilitiesAddress);

    // get stub
    DbusCapabilitiesStubAdapter* capaStub = new DbusCapabilitiesStubAdapter(ccCapabilitiesAddress);
    ASSERT_TRUE(capaStub->isProxyAvailabe());

    // call method
    QSharedPointer<DbusMessagingEndpointAddress> endPoint(new DbusMessagingEndpointAddress("defaultChannelId"));
    capaStub->addEndpoint(QString("local"), endPoint, 1000);

    // delete skeleton
    delete capaSkeleton;
    delete capaStub;
    delete capaMock;
}

TEST_F(LibJoynrDbusCommunicationTests, capabilities_call_add) {
    QString ccCapabilitiesAddress("local:joynr.capabilities:cc.message2");

    // register skeletons
    MockCapabilitiesStub* capaMock = new MockCapabilitiesStub();

    EXPECT_CALL(*capaMock, add(A<const QString&>(),
                               A<const QString&>(),
                               A<const QString&>(),
                               A<const types::ProviderQos&>(),
                               A<QList<QSharedPointer<EndpointAddressBase> >>(),
                               A<QSharedPointer<EndpointAddressBase>>(),
                               A<const qint64&>())).Times(1);
    auto capaSkeleton = new IDbusSkeletonWrapper<DbusCapabilitiesSkeleton, ICapabilities>(*capaMock, ccCapabilitiesAddress);

    // get stub
    DbusCapabilitiesStubAdapter* capaStub = new DbusCapabilitiesStubAdapter(ccCapabilitiesAddress);
    ASSERT_TRUE(capaStub->isProxyAvailabe());

    // call method
    QSharedPointer<DbusMessagingEndpointAddress> endPoint(new DbusMessagingEndpointAddress("defaultChannelId"));
    types::ProviderQos pQos;
    pQos.setPriority(5);
    capaStub->add(QString("local"), QString("interface"), QString("participantId"), pQos, QList<QSharedPointer<EndpointAddressBase>>(), endPoint, 1000);

    // delete skeleton
    delete capaSkeleton;
    delete capaStub;
    delete capaMock;
}

TEST_F(LibJoynrDbusCommunicationTests, capabilities_call_remove) {
    QString ccCapabilitiesAddress("local:joynr.capabilities:cc.message1");

    // register skeletons
    MockCapabilitiesStub* capaMock = new MockCapabilitiesStub();

    EXPECT_CALL(*capaMock, remove(A<const QString&>(), A<const qint64&>())).Times(1);
    auto capaSkeleton = new IDbusSkeletonWrapper<DbusCapabilitiesSkeleton, ICapabilities>(*capaMock, ccCapabilitiesAddress);

    // get stub
    DbusCapabilitiesStubAdapter* capaStub = new DbusCapabilitiesStubAdapter(ccCapabilitiesAddress);
    ASSERT_TRUE(capaStub->isProxyAvailabe());

    // call method
    capaStub->remove(QString("local"), 1000);

    // delete skeleton
    delete capaSkeleton;
    delete capaStub;
    delete capaMock;
}
