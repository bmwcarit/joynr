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
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "tests/utils/MockObjects.h"
#include "runtimes/cluster-controller-runtime/JoynrClusterControllerRuntime.h"
#include "runtimes/libjoynr-runtime/LibJoynrRuntime.h"
#include "joynr/MessagingSettings.h"
#include "joynr/SettingsMerger.h"
#include "joynr/LibjoynrSettings.h"
#include "joynr/HttpCommunicationManager.h"

#include "joynr/tests/DefaultTestProvider.h"
#include "joynr/tests/TestProxy.h"

#include "joynr/RequestStatus.h"
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"

#include "tests/utils/MockObjects.h"

#include "joynr/Future.h"

using namespace ::testing;

using namespace joynr;

class End2EndDbusTest : public Test {

public:
    QString messageSettingsFilename;
    QString libjoynrSettingsFilename;

    QSettings messagingQsettings;
    QSettings libjoynrQsettings;
    MessagingSettings* messagingSettings;
    LibjoynrSettings* libjoynrSettings;

    JoynrClusterControllerRuntime* clusterControllerRuntime;
    LibJoynrRuntime* runtime1;
    LibJoynrRuntime* runtime2;

    tests::TestProxy* testProxy;

    QString domain;
    QSemaphore semaphore;

    End2EndDbusTest() :
        messageSettingsFilename("test-resources/SystemIntegrationTest1.settings"),
        libjoynrSettingsFilename("test-resources/libjoynrintegrationtest.settings"),
        messagingQsettings(messageSettingsFilename, QSettings::IniFormat),
        libjoynrQsettings(libjoynrSettingsFilename, QSettings::IniFormat),
        messagingSettings(new MessagingSettings(messagingQsettings)),
        libjoynrSettings(new LibjoynrSettings(libjoynrQsettings)),
        clusterControllerRuntime(NULL),
        runtime1(NULL),
        runtime2(NULL),
        domain("local"),
        testProxy(NULL),
        semaphore(0)
    {
        // create the cluster controller runtime
        QSettings* settings = SettingsMerger::mergeSettings(messageSettingsFilename);
        SettingsMerger::mergeSettings(libjoynrSettingsFilename, settings);
        clusterControllerRuntime = new JoynrClusterControllerRuntime(NULL, settings,
                                           new HttpCommunicationManager(*messagingSettings));

        // create lib joynr runtimes
        runtime1 = new LibJoynrRuntime(new QSettings(libjoynrSettingsFilename, QSettings::IniFormat));
        runtime2 = new LibJoynrRuntime(new QSettings(libjoynrSettingsFilename, QSettings::IniFormat));
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

        delete messagingSettings;
        delete libjoynrSettings;
    }

    ~End2EndDbusTest(){
    }

    void registerTestProvider() {
        // create provider
        QString authenticationToken("authToken");

        types::ProviderQos providerQos;
        providerQos.setPriority(QDateTime::currentDateTime().toMSecsSinceEpoch());

        QSharedPointer<tests::TestProvider> provider(new MockTestProvider(providerQos));

        // register provider
        QString participantId = runtime1->registerCapability(domain, provider, authenticationToken);
        ASSERT_TRUE(participantId != NULL);
    }

    void connectProxy() {
        auto proxyBuilder = runtime2->getProxyBuilder<tests::TestProxy>(domain);
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
    semaphore->release(1);
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
    RequestStatus status;
    QString result;
    testProxy->sayHello(status, result);
    ASSERT_TRUE(status.successful());
    ASSERT_TRUE(result == "Hello World");
}

TEST_F(End2EndDbusTest, call_async_method)
{
    // register a provider
    registerTestProvider();

    // connect the proxy
    connectProxy();

    QSharedPointer<Future<QString>> sayHelloFuture(new Future<QString>());
    testProxy->sayHello(sayHelloFuture);
    sayHelloFuture->waitForFinished();
    ASSERT_TRUE(sayHelloFuture->isOk());
    ASSERT_EQ(sayHelloFuture->getValue(), "Hello World");
}

TEST_F(End2EndDbusTest, get_set_attribute_sync)
{
    // register a provider
    registerTestProvider();

    // connect the proxy
    connectProxy();

    // synchonous
    RequestStatus status;
    testProxy->setTestAttribute(status, 15);
    ASSERT_TRUE(status.successful());

    int result = 0;
    testProxy->getTestAttribute(status, result);
    ASSERT_TRUE(status.successful());
    ASSERT_EQ(15, result);
}

TEST_F(End2EndDbusTest, get_set_attribute_async)
{
    // register a provider
    registerTestProvider();

    // connect the proxy
    connectProxy();

    // asynchronous
    QSharedPointer<Future<void>> setAttributeFuture(new Future<void>());
    testProxy->setTestAttribute(setAttributeFuture, 18);
    setAttributeFuture->waitForFinished();
    ASSERT_TRUE(setAttributeFuture->isOk());

    QSharedPointer<Future<int>> getAttributeFuture(new Future<int>());
    testProxy->getTestAttribute(getAttributeFuture);
    getAttributeFuture->waitForFinished();
    ASSERT_TRUE(getAttributeFuture->isOk());
    ASSERT_EQ(getAttributeFuture->getValue(), 18);
}

TEST_F(End2EndDbusTest, subscriptionlistener)
{
    // register a provider
    registerTestProvider();

    // connect the proxy
    connectProxy();

    // use semaphore to count recieves
    auto mockListener = new MockSubscriptionListener<int>();
    EXPECT_CALL(*mockListener, receive(A<int>())).WillRepeatedly(ReleaseSemaphore(&semaphore));
    QSharedPointer<ISubscriptionListener<int> > subscriptionListener(mockListener);

    auto subscriptionQos = QSharedPointer<SubscriptionQos>(new OnChangeWithKeepAliveSubscriptionQos(
                500000, // validity_ms
                2000, // minInterval_ms
                3000, // maxInterval_ms
                4000 // alertInterval_ms
    ));
    testProxy->subscribeToTestAttribute(subscriptionListener, subscriptionQos);

    // Wait for 2 subscription messages to arrive
    ASSERT_TRUE(semaphore.tryAcquire(3, 20000));
}

TEST_F(End2EndDbusTest, performance_sendManyRequests) {
    // register a provider
    registerTestProvider();

    // connect the proxy
    connectProxy();

    qint64 startTime = QDateTime::currentMSecsSinceEpoch();
    QList<QSharedPointer<Future<int> > >testFutureList;
    int numberOfMessages = 100;
    int successFullMessages = 0;
    for (int i=0; i<numberOfMessages; i++){
        testFutureList.append(QSharedPointer<Future<int> >(new Future<int>() ) );
        QList<int> list;
        list.append(2);
        list.append(4);
        list.append(8);
        list.append(i);
        testProxy->sumInts(testFutureList.at(i), list);
    }

    for (int i=0; i<numberOfMessages; i++){
        testFutureList.at(i)->waitForFinished();
        int expectedValue = 2+4+8+i;
        if (testFutureList.at(i)->getStatus().successful()) {
            successFullMessages++;
            EXPECT_EQ(expectedValue, testFutureList.at(i)->getValue());
        }
    }
    qint64 stopTime = QDateTime::currentMSecsSinceEpoch();
    //check if all Messages were received:
    EXPECT_EQ(numberOfMessages, successFullMessages);
    Logger* logger = Logging::getInstance()->getLogger("TEST", "End2EndDbusTest");
    LOG_INFO(logger,"Required Time for " + QString::number(numberOfMessages) + " Messages: " + QString::number(stopTime - startTime));
}
