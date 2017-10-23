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
#include <gtest/gtest.h>
#include "joynr/MessagingSettings.h"
#include "tests/utils/MockObjects.h"
#include "joynr/IDbusSkeletonWrapper.h"
#include "common/dbus/DbusMessagingStubAdapter.h"
#include "joynr/DbusMessagingSkeleton.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/system/RoutingTypes/CommonApiDbusAddress.h"

#include "tests/utils/MockObjects.h"
#include "joynr/IMessaging.h"
#include "joynr/Settings.h"
#include "joynr/Logger.h"

#include <string>
#include <chrono>

using namespace joynr;

class LibJoynrDbusCommunicationTests : public testing::Test {

public:

    std::string settingsFilename;
    Settings settings;
    MessagingSettings* messagingSettings;
    ADD_LOGGER(LibJoynrDbusCommunicationTests)

    LibJoynrDbusCommunicationTests():
        settingsFilename("test-resources/integrationtest.settings"),
        settings(settingsFilename),
        messagingSettings(new MessagingSettings(settings))
    {
    }

    ~LibJoynrDbusCommunicationTests() {
    }

    void SetUp() {
    }

    void printResult(Logger& logger, const std::string& text, bool result) {
        if(result) JOYNR_LOG_ERROR(logger, "{} SUCCESS", text); else JOYNR_LOG_ERROR(logger, "{} ERROR", text);
    }
};

TEST_F(LibJoynrDbusCommunicationTests, dbus_commonapi_runtime_feature_check) {
    std::string ccMessagingAddress("local:cc.messaging:cc.messaging8");

    // create the skeleton
    MockMessaging* msgMock = new MockMessaging();
    EXPECT_CALL(*msgMock, transmit(A<JoynrMessage&>())).Times(2);
    auto provider = std::make_shared<DbusMessagingSkeleton>(*msgMock);

    // register skeleton
    auto runtime = CommonAPI::Runtime::load("DBus");
    auto factory = runtime->createFactory();

    bool success = runtime->getServicePublisher()->registerService(provider, ccMessagingAddress, factory);
    ASSERT_TRUE(success);
    printResult(logger, "registerService", success);

    // get proxy
    auto runtime2 = CommonAPI::Runtime::load("DBus");
    auto factory2 = runtime2->createFactory();
    auto proxy = factory2->buildProxy<joynr::messaging::IMessagingProxy>(ccMessagingAddress);
    // wait some time so that the proxy is ready to use on dbus level
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // call method
    joynr::messaging::IMessaging::JoynrMessage message;
    message.type =  "reply";
    message.payload = "This is a test";
    CommonAPI::CallStatus status;
    ASSERT_TRUE(proxy->isAvailable());
    proxy->transmit(message, status);
    printResult(logger, "transmit status", status == CommonAPI::CallStatus::SUCCESS);
    ASSERT_TRUE(status == CommonAPI::CallStatus::SUCCESS);

    // unregister service
    success = runtime->getServicePublisher()->unregisterService(ccMessagingAddress);
    printResult(logger, "unregisterService", success);
    ASSERT_TRUE(success);

    // call method
    proxy->transmit(message, status);
    printResult(logger, "transmit status", status == CommonAPI::CallStatus::SUCCESS);
    ASSERT_FALSE(status == CommonAPI::CallStatus::SUCCESS);

    printResult(logger, "proxy available:", proxy->isAvailable());

    // register service
    success = runtime->getServicePublisher()->registerService(provider, ccMessagingAddress, factory);
    printResult(logger, "registerService", success);
    ASSERT_TRUE(success);
    // wait some time so that the service is registered on dbus level
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // call method
    proxy->transmit(message, status);
    printResult(logger, "transmit status", status == CommonAPI::CallStatus::SUCCESS);
    ASSERT_TRUE(status == CommonAPI::CallStatus::SUCCESS);

    // unregister service
    success = runtime->getServicePublisher()->unregisterService(ccMessagingAddress);
    printResult(logger, "unregisterService", success);
    ASSERT_TRUE(success);

    delete msgMock;
}

TEST_F(LibJoynrDbusCommunicationTests, dbus_skeletonwrapper_register_unregister) {
    std::string ccMessagingAddress("local:cc.messaging:cc.messaging8");

    // craete mock and expect 2 calls
    MockMessaging* msgMock = new MockMessaging();
    EXPECT_CALL(*msgMock, transmit(A<JoynrMessage&>())).Times(2);

    // create the skeleton
    JOYNR_LOG_INFO(logger, "Register skeleton");
    auto msgSkeleton = new IDbusSkeletonWrapper<DbusMessagingSkeleton, IMessaging>(*msgMock, ccMessagingAddress);

    // create message
    JoynrMessage msg;
    msg.setType(JoynrMessage::VALUE_MESSAGE_TYPE_ONE_WAY);
    msg.setHeaderTo("local");
    msg.setPayload("This is a test");

    // get stub
    DbusMessagingStubAdapter* msgStub = new DbusMessagingStubAdapter(ccMessagingAddress);
    ASSERT_TRUE(msgStub->isProxyAvailable());

    // call method
    JOYNR_LOG_INFO(logger, "Transmit message: should work");
    msgStub->transmit(msg);

    // delete skeleton
    JOYNR_LOG_INFO(logger, "Delete skeleton");
    delete msgSkeleton;

    // call method
    JOYNR_LOG_INFO(logger, "Transmit message: should fail");
    msgStub->transmit(msg);

    // register skeleton
    JOYNR_LOG_INFO(logger, "Register skeleton");
    msgSkeleton = new IDbusSkeletonWrapper<DbusMessagingSkeleton, IMessaging>(*msgMock, ccMessagingAddress);

    // call method
    JOYNR_LOG_INFO(logger, "Transmit message: should work");
    msgStub->transmit(msg);

    delete msgSkeleton;

    delete msgMock;
}

TEST_F(LibJoynrDbusCommunicationTests, DISABLED_connection_test) {
    std::string ccMessagingAddress = "local:cc.messaging:cc.messaging7";

    // register skeletons
    MockMessaging* msgMock = new MockMessaging();
    auto msgSkeleton = new IDbusSkeletonWrapper<DbusMessagingSkeleton, IMessaging>(*msgMock, ccMessagingAddress);

    // get stub
    DbusMessagingStubAdapter* msgStub = new DbusMessagingStubAdapter(ccMessagingAddress);
    ASSERT_TRUE(msgStub->isProxyAvailable());

    // delete skeleton
    delete msgSkeleton;

    ASSERT_FALSE(msgStub->isProxyAvailable());
    delete msgStub;
}

TEST_F(LibJoynrDbusCommunicationTests, transmit_message) {
    std::string ccMessagingAddress("local:joynr.messaging:cc.message6");

    // register skeletons
    MockMessaging* msgMock = new MockMessaging();
    EXPECT_CALL(*msgMock, transmit(A<JoynrMessage&>())).Times(1);
    auto msgSkeleton = new IDbusSkeletonWrapper<DbusMessagingSkeleton, IMessaging>(*msgMock, ccMessagingAddress);

    // get stub
    DbusMessagingStubAdapter* msgStub = new DbusMessagingStubAdapter(ccMessagingAddress);
    ASSERT_TRUE(msgStub->isProxyAvailable());

    // create message
    JoynrMessage msg;
    msg.setType(JoynrMessage::VALUE_MESSAGE_TYPE_ONE_WAY);
    msg.setHeaderTo("local");
    msg.setPayload("This is a test");

    // create messaging qos
    msgStub->transmit(msg);

    // delete skeleton
    delete msgSkeleton;

    // error on transmission
    msgStub->transmit(msg);

    // stub not availabe
    ASSERT_FALSE(msgStub->isProxyAvailable());

    delete msgStub;
    delete msgMock;
}
