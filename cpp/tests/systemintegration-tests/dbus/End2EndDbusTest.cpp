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
#include "joynr/PrivateCopyAssign.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>
#include <stdint.h>
#include "joynr/DispatcherUtils.h"
#include "tests/utils/MockObjects.h"
#include "runtimes/cluster-controller-runtime/JoynrClusterControllerRuntime.h"
#include "runtimes/libjoynr-runtime/dbus/LibJoynrDbusRuntime.h"

#include "joynr/tests/DefaulttestProvider.h"
#include "joynr/tests/testProxy.h"

#include "joynr/RequestStatus.h"
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"

#include "tests/utils/MockObjects.h"

#include "joynr/Future.h"
#include "joynr/Logger.h"

using namespace ::testing;

using namespace joynr;

class End2EndDbusTest : public Test {

public:
    std::string messageSettingsFilename;

    JoynrClusterControllerRuntime* clusterControllerRuntime;
    LibJoynrDbusRuntime* runtime1;
    LibJoynrDbusRuntime* runtime2;

    tests::testProxy* testProxy;

    std::string domain;
    joynr::Semaphore semaphore;

    End2EndDbusTest() :
        messageSettingsFilename("test-resources/HttpSystemIntegrationTest1.settings"),
        clusterControllerRuntime(NULL),
        runtime1(NULL),
        runtime2(NULL),
        testProxy(NULL),
        domain("local"),
        semaphore(0)
    {
        // create the cluster controller runtime
        clusterControllerRuntime = new JoynrClusterControllerRuntime(
                    NULL,
                    new Settings(messageSettingsFilename)
        );
        clusterControllerRuntime->registerRoutingProvider();
        clusterControllerRuntime->registerDiscoveryProvider();

        // create lib joynr runtimes
        runtime1 = new LibJoynrDbusRuntime(new Settings(messageSettingsFilename));
        runtime2 = new LibJoynrDbusRuntime(new Settings(messageSettingsFilename));
    }

    void SetUp() {
        clusterControllerRuntime->startMessaging();
        clusterControllerRuntime->waitForChannelCreation();
    }

    void TearDown() {
        if(testProxy != NULL) delete testProxy;
        if(runtime1 != NULL) delete runtime1;
        if(runtime2 != NULL) delete runtime2;

        if(clusterControllerRuntime != NULL) {
            clusterControllerRuntime->deleteChannel();
            clusterControllerRuntime->stopMessaging();
            delete clusterControllerRuntime;
        }
    }

    ~End2EndDbusTest(){
    }

    void registerTestProvider() {
        // create provider
        types::ProviderQos providerQos;
        providerQos.setPriority(DispatcherUtils::nowInMilliseconds());

        auto provider = std::make_shared<MockTestProvider>(providerQos);

        // register provider
        std::string participantId = runtime1->registerProvider(domain, provider);
        ASSERT_TRUE(!participantId.empty());
    }

    void connectProxy() {
        auto proxyBuilder = runtime2->createProxyBuilder<tests::testProxy>(domain);
        ASSERT_TRUE(proxyBuilder != NULL);

        // start arbitration
        DiscoveryQos discoveryQos;
        discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
        discoveryQos.setDiscoveryTimeout(20000);
        proxyBuilder->setDiscoveryQos(discoveryQos);

        testProxy = proxyBuilder->build();
        ASSERT_TRUE(testProxy != NULL);

        delete proxyBuilder;
    }

private:
    DISALLOW_COPY_AND_ASSIGN(End2EndDbusTest);

};


ACTION_P(ReleaseSemaphore,semaphore)
{
    semaphore->notify();
}

TEST_F(End2EndDbusTest, instantiate_Runtimes)
{
    ASSERT_TRUE(clusterControllerRuntime != NULL);
    ASSERT_TRUE(runtime1 != NULL);
    ASSERT_TRUE(runtime2 != NULL);
}


TEST_F(End2EndDbusTest, call_sync_method)
{
    // register a provider
    registerTestProvider();

    // connect the proxy
    connectProxy();

    // call method
    std::string actualValue;
    try {
        testProxy->sayHello(actualValue);
        ASSERT_EQ("Hello World", actualValue);
    } catch (exceptions::JoynrException& e) {
        ADD_FAILURE()<< "sayHello was not successful";
    }
}

TEST_F(End2EndDbusTest, call_async_method)
{
    // register a provider
    registerTestProvider();

    // connect the proxy
    connectProxy();

    std::shared_ptr<Future<std::string>> sayHelloFuture(testProxy->sayHelloAsync());
    sayHelloFuture->wait();
    ASSERT_TRUE(sayHelloFuture->isOk());
    std::string actualValue;
    sayHelloFuture->get(actualValue);
    ASSERT_EQ("Hello World", actualValue);
}

TEST_F(End2EndDbusTest, get_set_attribute_sync)
{
    // register a provider
    registerTestProvider();

    // connect the proxy
    connectProxy();

    // synchronous
    try {
        testProxy->setTestAttribute(15);
    } catch (exceptions::JoynrException& e) {
        ADD_FAILURE()<< "setTestAttribute was not successful";
    }

    int result = 0;
    try {
        testProxy->getTestAttribute(result);
    } catch (exceptions::JoynrException& e) {
        ADD_FAILURE()<< "getTestAttribute was not successful";
    }
    ASSERT_EQ(15, result);
}

TEST_F(End2EndDbusTest, get_set_attribute_async)
{
    // register a provider
    registerTestProvider();

    // connect the proxy
    connectProxy();

    // asynchronous
    std::shared_ptr<Future<void>> setAttributeFuture(testProxy->setTestAttributeAsync(18));
    setAttributeFuture->wait();
    ASSERT_TRUE(setAttributeFuture->isOk());

    std::shared_ptr<Future<int>> getAttributeFuture(testProxy->getTestAttributeAsync());
    getAttributeFuture->wait();
    ASSERT_TRUE(getAttributeFuture->isOk());
    int actualValue;
    getAttributeFuture->get(actualValue);
    ASSERT_EQ(18, actualValue);
}

TEST_F(End2EndDbusTest, subscriptionlistener)
{
    // register a provider
    registerTestProvider();

    // connect the proxy
    connectProxy();

    // use semaphore to count recieves
    auto mockListener = new MockSubscriptionListenerOneType<int>();
    EXPECT_CALL(*mockListener, onReceive(A<const int&>())).WillRepeatedly(ReleaseSemaphore(&semaphore));
    std::shared_ptr<ISubscriptionListener<int> > subscriptionListener(mockListener);

    OnChangeWithKeepAliveSubscriptionQos subscriptionQos(
                500000, // validity_ms
                2000, // minInterval_ms
                3000, // maxInterval_ms
                4000 // alertInterval_ms
    );
    testProxy->subscribeToTestAttribute(subscriptionListener, subscriptionQos);

    // Wait for 2 subscription messages to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(20)));
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(20)));
}

TEST_F(End2EndDbusTest, performance_sendManyRequests) {
    // register a provider
    registerTestProvider();

    // connect the proxy
    connectProxy();

    uint64_t startTime = DispatcherUtils::nowInMilliseconds();
    std::vector<std::shared_ptr<Future<int32_t> > >testFutureList;
    int numberOfMessages = 500;
    int successFullMessages = 0;
    for (int32_t i=0; i<numberOfMessages; i++){
        std::vector<int32_t> list;
        list.push_back(2);
        list.push_back(4);
        list.push_back(8);
        list.push_back(i);
        testFutureList.push_back(testProxy->sumIntsAsync(list));
    }

    for (int i=0; i<numberOfMessages; i++){
        try {
            testFutureList.at(i)->wait(25 * numberOfMessages);
            int32_t expectedValue = 2+4+8+i;
            if (testFutureList.at(i)->getStatus().successful()) {
                successFullMessages++;
                int actualValue;
                testFutureList.at(i)->get(actualValue);
                EXPECT_EQ(expectedValue, actualValue);
            }
        } catch (exceptions::JoynrTimeOutException& e) {
            ADD_FAILURE()<< "Timeout waiting for response";
        }
    }

    uint64_t stopTime = DispatcherUtils::nowInMilliseconds();
    //check if all Messages were received:
    EXPECT_EQ(numberOfMessages, successFullMessages);
    Logger logger("End2EndDbusTest");
    JOYNR_LOG_INFO(logger, "Required Time for {} Messages: ", numberOfMessages, (stopTime - startTime) );
}
