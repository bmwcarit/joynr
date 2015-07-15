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
#include <memory>
#include <string>
#include <QtConcurrent/QtConcurrent>
#include "tests/utils/MockObjects.h"
#include "runtimes/cluster-controller-runtime/JoynrClusterControllerRuntime.h"
#include "joynr/tests/testProxy.h"
#include "joynr/tests/DerivedStruct.h"
#include "joynr/tests/AnotherDerivedStruct.h"
#include "joynr/types/Trip.h"
#include "joynr/types/GpsLocation.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/types/CapabilityInformation.h"
#include "joynr/CapabilitiesRegistrar.h"
#include "utils/QThreadSleep.h"
#include "PrettyPrint.h"
#include "joynr/LocalCapabilitiesDirectory.h"
#include "joynr/MessagingSettings.h"
#include "joynr/Future.h"
#include "joynr/SettingsMerger.h"
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"
#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/LocalChannelUrlDirectory.h"

using namespace ::testing;
using namespace joynr;
using namespace joynr_logging;

ACTION_P(ReleaseSemaphore,semaphore)
{
    semaphore->release(1);
}


static const QString messagingPropertiesPersistenceFileName1("CombinedEnd2EndTest-runtime1-joynr.settings");
static const QString messagingPropertiesPersistenceFileName2("CombinedEnd2EndTest-runtime2-joynr.settings");


/*
  * This test tries to create two combined Runtimes and will test communication
  * between the two Runtimes via HttpReceiver
  *
  */

class CombinedEnd2EndTest : public Test {
public:
    types::ProviderQos qRegisterMetaTypeQos; //this is necessary to force a qRegisterMetaType<types::ProviderQos>(); during setup
    types::CapabilityInformation qRegisterMetaTypeCi; //this is necessary to force a qRegisterMetaType<types::ProviderQos>(); during setup
    JoynrClusterControllerRuntime* runtime1;
    JoynrClusterControllerRuntime* runtime2;
    QString registeredSubscriptionId;
    QSettings settings1;
    QSettings settings2;
    MessagingSettings messagingSettings1;
    MessagingSettings messagingSettings2;
    std::string baseUuid;
    std::string uuid;
    std::string domainName;
    QSemaphore semaphore;

    CombinedEnd2EndTest() :
        qRegisterMetaTypeQos(),
        qRegisterMetaTypeCi(),
        runtime1(NULL),
        runtime2(NULL),
        settings1("test-resources/SystemIntegrationTest1.settings", QSettings::IniFormat),
        settings2("test-resources/SystemIntegrationTest2.settings", QSettings::IniFormat),
        messagingSettings1(settings1),
        messagingSettings2(settings2),
        baseUuid(TypeUtil::convertQStringtoStdString(QUuid::createUuid().toString())),
        uuid( "_" + baseUuid.substr(1, baseUuid.length()-2)),
        domainName("cppCombinedEnd2EndTest_Domain" + uuid),
        semaphore(0)
    {
        messagingSettings1.setMessagingPropertiesPersistenceFilename(messagingPropertiesPersistenceFileName1);
        messagingSettings2.setMessagingPropertiesPersistenceFilename(messagingPropertiesPersistenceFileName2);

        //This is a workaround to register the Metatypes for providerQos.
        //Normally a new datatype is registered in all datatypes that use the new datatype.
        //However, when receiving a datatype as a returnValue of a RPC, the constructor has never been called before
        //so the datatype is not registered, and cannot be deserialized.
        joynr::types::ProviderQos a;
        joynr__types__ProviderQos b;
//        qRegisterMetaType<types::ProviderQos>("types::ProviderQos");
//        //TODO: also remove the variables qRegisterMetaTypeQos
//        qRegisterMetaType<types__ProviderQos>("types__ProviderQos");

        QSettings* settings_1 = SettingsMerger::mergeSettings(QString("test-resources/SystemIntegrationTest1.settings"));
        SettingsMerger::mergeSettings(QString("test-resources/libjoynrSystemIntegration1.settings"), settings_1);
        runtime1 = new JoynrClusterControllerRuntime(NULL, settings_1);
        QSettings* settings_2 = SettingsMerger::mergeSettings(QString("test-resources/SystemIntegrationTest2.settings"));
        SettingsMerger::mergeSettings(QString("test-resources/libjoynrSystemIntegration2.settings"), settings_2);
        runtime2 = new JoynrClusterControllerRuntime(NULL, settings_2);
    }

    void SetUp() {
        runtime1->start();
        runtime2->start();
    }

    void TearDown() {
        bool deleteChannel = true;
        runtime1->stop(deleteChannel);
        runtime2->stop(deleteChannel);

        // Delete the persisted participant ids so that each test uses different participant ids
        QFile::remove(LibjoynrSettings::DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME());
    }

    ~CombinedEnd2EndTest(){
        delete runtime1;
        delete runtime2;
    }

private:
    DISALLOW_COPY_AND_ASSIGN(CombinedEnd2EndTest);

};

TEST_F(CombinedEnd2EndTest, callRpcMethodViaHttpReceiverAndReceiveReply) {

    // Provider: (runtime1)
    //This is a workaround to register the Metatypes for providerQos.
    //Normally a new datatype is registered in all datatypes that use the new datatype.
    //However, when receiving a datatype as a returnValue of a RPC, the constructor has never been called before
    //so the datatype is not registered, and cannot be deserialized.
    qRegisterMetaType<types::ProviderQos>("types::ProviderQos");
    //TODO: also remove the variables qRegisterMetaTypeQos
    types::ProviderQos();
//    qRegisterMetaType<types__ProviderQos>("types__ProviderQos");


    types::ProviderQos providerQos;
    providerQos.setPriority(2);
    std::shared_ptr<tests::testProvider> testProvider(new MockTestProvider(providerQos));

    QThreadSleep::msleep(1000);

    runtime1->registerProvider<tests::testProvider>(domainName, testProvider);

    QThreadSleep::msleep(1000);

    //consumer for testinterface
    // Testing Lists
    {
        ProxyBuilder<tests::testProxy>* testProxyBuilder = runtime2->createProxyBuilder<tests::testProxy>(domainName);
        DiscoveryQos discoveryQos;
        discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
        discoveryQos.setDiscoveryTimeout(1000);

        qlonglong qosRoundTripTTL = 40000;

        // Send a message and expect to get a result
        QSharedPointer<tests::testProxy> testProxy(testProxyBuilder
                                                   ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                                   ->setCached(false)
                                                   ->setDiscoveryQos(discoveryQos)
                                                   ->build());

        QList<int> list;
        list.append(2);
        list.append(4);
        list.append(8);
        QSharedPointer<Future<int> >gpsFuture (testProxy->sumInts(list));
        gpsFuture->waitForFinished();
        int expectedValue = 2+4+8;
        ASSERT_TRUE(gpsFuture->getStatus().successful());
        int actualValue;
        gpsFuture->getValues(actualValue);
        EXPECT_EQ(expectedValue, actualValue);
        //TODO CA: shared pointer for proxy builder?

     /*
      * Testing TRIP
      * Now try to send a Trip (which contains a list) and check if the returned trip is identical.
      */

        QList<types::GpsLocation> inputLocationList;
        inputLocationList.append(types::GpsLocation(1.1, 2.2, 3.3, types::GpsFixEnum::MODE2D, 0.0, 0.0, 0.0, 0.0, 444, 444, 4));
        inputLocationList.append(types::GpsLocation(1.1, 2.2, 3.3, types::GpsFixEnum::MODE2D, 0.0, 0.0, 0.0, 0.0, 444, 444, 5));
        inputLocationList.append(types::GpsLocation(1.1, 2.2, 3.3, types::GpsFixEnum::MODE2D, 0.0, 0.0, 0.0, 0.0, 444, 444, 6));
        types::Trip inputTrip;
        inputTrip.setLocations(inputLocationList);
        QSharedPointer<Future<types::Trip> > tripFuture (testProxy->optimizeTrip(inputTrip));
        tripFuture->waitForFinished();
        ASSERT_EQ(RequestStatusCode::OK, tripFuture->getStatus().getCode());
        types::Trip actualTrip;
        tripFuture->getValues(actualTrip);
        EXPECT_EQ(inputTrip, actualTrip);

     /*
      * Testing Lists in returnvalues
      * Now try to send call a method that has a list as return parameter
      */

        QList<int> primesBelow6;
        primesBelow6.append(2);
        primesBelow6.append(3);
        primesBelow6.append(5);
        RequestStatus rs;
        QList<int> result;
        testProxy->returnPrimeNumbers(rs, result, 6);
        if (rs.successful()) {
            EXPECT_EQ(primesBelow6, result);
        } else {
            FAIL() << "Requeststatus was not successful";
        }

        delete testProxyBuilder;

    /*
     * Testing List of Vowels
     * Now try to send a List of Vowels and see if it is returned correctly.
     */
// does not compile, see 654
//       QList<Vowel> inputWord;
//       inputWord.append("h");
//       inputWord.append("e");
//       inputWord.append("l");
//       inputWord.append("l");
//       inputWord.append("o");
//       QSharedPointer<Future<QList<Vowel> > > wordFuture (new QSharedPointer<Future<QList<Vowel> > >());
//       testProxy->optimizeWord(wordFuture, inputTrip);
//       wordFuture->waitForFinished();
//       ASSERT_EQ(RequestStatusCode::OK, wordFuture->getStatus().getCode());
//       inputTrip.append("a"); //thats what optimize word does.. appending an a.
//       EXPECT_EQ(inputTrip, tripFuture->getValue());

    /*
     * Testing List of Locations
     * Now try to send a List of GpsLocations and see if it is returned correctly.
     */
        // is currently deactivated, because it throws an assertion.
        QList<types::GpsLocation> inputGpsLocationList;
        inputGpsLocationList.append(types::GpsLocation(1.1, 2.2, 3.3, types::GpsFixEnum::MODE2D, 0.0, 0.0, 0.0, 0.0, 444, 444, 4));
        inputGpsLocationList.append(types::GpsLocation(1.1, 2.2, 3.3, types::GpsFixEnum::MODE2D, 0.0, 0.0, 0.0, 0.0, 444, 444, 5));
        inputGpsLocationList.append(types::GpsLocation(1.1, 2.2, 3.3, types::GpsFixEnum::MODE2D, 0.0, 0.0, 0.0, 0.0, 444, 444, 6));
        QSharedPointer<Future<QList<types::GpsLocation> > > listLocationFuture (testProxy->optimizeLocationList(inputGpsLocationList));
        listLocationFuture->waitForFinished();
        ASSERT_EQ(RequestStatusCode::OK, listLocationFuture->getStatus().getCode());
        QList<joynr::types::GpsLocation> actualLocation;
        listLocationFuture->getValues(actualLocation);
        EXPECT_EQ(inputGpsLocationList, actualLocation);

     /*
      * Testing GetAttribute, when setAttribute has been called locally.
      *
      */
        // Primes
        int testPrimeValue = 15;
        RequestStatus status;
        RequestStatus statusOfSet;

        std::function<void(const joynr::RequestStatus& status)> callbackFct =
                [] (const joynr::RequestStatus& status) {};

        /*
         * because of the implementation of the MockTestProvider,
         * we can use the async API of the testProvider in a sync way
         */
        testProvider->setFirstPrime(testPrimeValue, callbackFct);

        int primeResult(0);
        testProxy->getFirstPrime(status, primeResult);
        ASSERT_TRUE(status.successful());
        EXPECT_EQ(primeResult, 15);

        // List of strings,
        QList<QString> localStrList;
        QList<QString> remoteStrList;
        localStrList.append("one ü");
        localStrList.append("two 漢語");
        localStrList.append("three ـتـ");
        localStrList.append("four {");
        testProvider->setListOfStrings(localStrList, callbackFct);

        testProxy->getListOfStrings(status, remoteStrList);
        ASSERT_TRUE(status.successful());
        EXPECT_EQ(localStrList, remoteStrList);

     /*
      * Testing GetAttribute with Remote SetAttribute
      *
      */

        testProxy->setFirstPrime(statusOfSet, 19);
        ASSERT_TRUE(statusOfSet.successful());
        testProxy->getFirstPrime(status, primeResult);
        ASSERT_TRUE(status.successful());
        EXPECT_EQ(primeResult, 19);


      /*
        *
        * Testing local/remote getters and setters with lists.
        */

        QList<int> inputIntList;
        inputIntList<<2<<3<<5;
        testProvider->setListOfInts(inputIntList, callbackFct);
        QList<int> outputIntLIst;
        testProxy->getListOfInts(status, outputIntLIst);
        ASSERT_TRUE(status.successful());
        EXPECT_EQ(outputIntLIst, inputIntList);
        EXPECT_EQ(outputIntLIst.at(1), 3);
        //test remote setter
        inputIntList.clear();
        inputIntList<<7<<11<<13;
        testProxy->setListOfInts(statusOfSet, inputIntList);
        testProxy->getListOfInts(status, outputIntLIst);
        ASSERT_TRUE(status.successful());
        EXPECT_EQ(outputIntLIst, inputIntList);
        EXPECT_EQ(outputIntLIst.at(1), 11);
    }

    // Testing TTL
    {
       //create a proxy with very short TTL and expect no returning replies.
        ProxyBuilder<tests::testProxy>* testProxyBuilder = runtime2->createProxyBuilder<tests::testProxy>(domainName);
        DiscoveryQos discoveryQos;
        discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
        discoveryQos.setDiscoveryTimeout(1000);

        qlonglong qosRoundTripTTL = 1;
        QSharedPointer<tests::testProxy> testProxy(testProxyBuilder
                                                   ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                                   ->setCached(false)
                                                   ->setDiscoveryQos(discoveryQos)
                                                   ->build());
        QSharedPointer<Future<int> > testFuture(testProxy->addNumbers(1, 2, 3));
        testFuture->waitForFinished();
        ASSERT_EQ(testFuture->getStatus().getCode(), RequestStatusCode::ERROR_TIME_OUT_WAITING_FOR_RESPONSE);
        //TODO CA: shared pointer for proxy builder?
        delete testProxyBuilder;
    }

    // Operation overloading is not currently supported
#if 0
    // Testing operation overloading
    {
        ProxyBuilder<tests::TestProxy>* testProxyBuilder = runtime2->createProxyBuilder<tests::testProxy>(domainName);
        DiscoveryQos discoveryQos;
        discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HighestPriority);
        discoveryQos.setDiscoveryTimeout(1000);

        qlonglong qosOneWayTTL = 40000;
        qlonglong qosRoundTripTTL = 40000;

        // Send a message and expect to get a result
        QSharedPointer<tests::testProxy> testProxy(testProxyBuilder
                                                   ->setMessagingQos(MessagingQos(qosOneWayTTL, qosRoundTripTTL))
                                                   ->setCached(false)
                                                   ->setDiscoveryQos(discoveryQos)
                                                   ->build());

        RequestStatus status;
        QString derivedStructResult;
        QString anotherDerivedStructResult;

        // Check that the operation overloading worked and the result is of the correct type
        testProxy->overloadedOperation(status, derivedStructResult, tests::DerivedStruct());
        testProxy->overloadedOperation(status, anotherDerivedStructResult, tests::AnotherDerivedStruct());
        EXPECT_EQ(derivedStructResult, "DerivedStruct");
        EXPECT_EQ(anotherDerivedStructResult, "AnotherDerivedStruct");
    }
#endif

}


TEST_F(CombinedEnd2EndTest, subscribeViaHttpReceiverAndReceiveReply) {

    //This is a workaround to register the Metatypes for providerQos.
    //Normally a new datatype is registered in all datatypes that use the new datatype.
    //However, when receiving a datatype as a returnValue of a RPC, the constructor has never been called before
    //so the datatype is not registered, and cannot be deserialized.
    qRegisterMetaType<types::ProviderQos>("types::ProviderQos");

    MockGpsSubscriptionListener* mockListener = new MockGpsSubscriptionListener();

    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener, onReceive(A<const types::GpsLocation&>()))
            .WillRepeatedly(ReleaseSemaphore(&semaphore));

    QSharedPointer<ISubscriptionListener<types::GpsLocation> > subscriptionListener(
                    mockListener);
    // Provider: (runtime1)

    types::ProviderQos providerQos;
    providerQos.setPriority(2);
    std::shared_ptr<tests::testProvider> testProvider(new tests::DefaulttestProvider(providerQos));
    //MockGpsProvider* gpsProvider = new MockGpsProvider();
    types::GpsLocation gpsLocation1;
    runtime1->registerProvider<tests::testProvider>(domainName, testProvider);

    //This wait is necessary, because registerProvider is async, and a lookup could occur
    // before the register has finished.
    QThreadSleep::msleep(5000);

    ProxyBuilder<tests::testProxy>* testProxyBuilder
            = runtime2->createProxyBuilder<tests::testProxy>(domainName);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeout(1000);

    qlonglong qosRoundTripTTL = 40000;

    // Send a message and expect to get a result
    QSharedPointer<tests::testProxy> testProxy(testProxyBuilder
                                               ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                               ->setCached(false)
                                               ->setDiscoveryQos(discoveryQos)
                                               ->build());

    qint64 minInterval_ms = 1000;
    qint64 maxInterval_ms = 2000;

    QSharedPointer<SubscriptionQos> subscriptionQos(new OnChangeWithKeepAliveSubscriptionQos(
                                    500000,   // validity_ms
                                    minInterval_ms,
                                    maxInterval_ms,
                                    3000));  // alertInterval_ms
    QString subscriptionId = testProxy->subscribeToLocation(subscriptionListener, subscriptionQos);

    // Wait for 2 subscription messages to arrive
    ASSERT_TRUE(semaphore.tryAcquire(2, 20000));

    delete testProxyBuilder;
}

TEST_F(CombinedEnd2EndTest, subscribeToOnChange) {

    //This is a workaround to register the Metatypes for providerQos.
    //Normally a new datatype is registered in all datatypes that use the new datatype.
    //However, when receiving a datatype as a returnValue of a RPC, the constructor has never been called before
    //so the datatype is not registered, and cannot be deserialized.
    qRegisterMetaType<types::ProviderQos>("types::ProviderQos");

    MockGpsSubscriptionListener* mockListener = new MockGpsSubscriptionListener();

    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener, onReceive(A<const types::GpsLocation&>()))
            .WillRepeatedly(ReleaseSemaphore(&semaphore));

    QSharedPointer<ISubscriptionListener<types::GpsLocation> > subscriptionListener(
                    mockListener);
    // Provider: (runtime1)

    types::ProviderQos providerQos;
    providerQos.setPriority(2);
    providerQos.setSupportsOnChangeSubscriptions(true);
    std::shared_ptr<tests::testProvider> testProvider(new tests::DefaulttestProvider(providerQos));
    runtime1->registerProvider<tests::testProvider>(domainName, testProvider);

    //This wait is necessary, because registerProvider is async, and a lookup could occur
    // before the register has finished.
    QThreadSleep::msleep(5000);

    ProxyBuilder<tests::testProxy>* testProxyBuilder
            = runtime2->createProxyBuilder<tests::testProxy>(domainName);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeout(1000);

    qlonglong qosRoundTripTTL = 40000;

    // Send a message and expect to get a result
    QSharedPointer<tests::testProxy> testProxy(testProxyBuilder
                                               ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                               ->setCached(false)
                                               ->setDiscoveryQos(discoveryQos)
                                               ->build());

    // Publications will be sent maintaining this minimum interval provided, even if the value
    // changes more often. This prevents the consumer from being flooded by updated values.
    // The filtering happens on the provider's side, thus also preventing excessive network traffic.
    // This value is provided in milliseconds. The minimum value for minInterval is 50 ms.
    qint64 minInterval_ms = 50;
    QSharedPointer<SubscriptionQos> subscriptionQos(new OnChangeSubscriptionQos(
                                    500000,   // validity_ms
                                    minInterval_ms));  // minInterval_ms
    QString subscriptionId = testProxy->subscribeToLocation(subscriptionListener, subscriptionQos);

    //This wait is necessary, because subcriptions are async, and an attribute could be changed before
    // before the subscription has started.
    QThreadSleep::msleep(5000);

    // Change the location once
    RequestStatus requestStatus;
    testProxy->setLocation(requestStatus,
                          types::GpsLocation(9.0, 51.0, 508.0, types::GpsFixEnum::MODE2D, 0.0, 0.0, 0.0, 0.0, 444, 444, 1));

    // Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.tryAcquire(1, 20000));

    // Change the location 3 times
    testProxy->setLocation(requestStatus,
                          types::GpsLocation(9.0, 51.0, 508.0, types::GpsFixEnum::MODE2D, 0.0, 0.0, 0.0, 0.0, 444, 444, 2));
    QThreadSleep::msleep(minInterval_ms + 50);
    testProxy->setLocation(requestStatus,
                          types::GpsLocation(9.0, 51.0, 508.0, types::GpsFixEnum::MODE2D, 0.0, 0.0, 0.0, 0.0, 444, 444, 3));
    QThreadSleep::msleep(minInterval_ms + 50);
    testProxy->setLocation(requestStatus,
                          types::GpsLocation(9.0, 51.0, 508.0, types::GpsFixEnum::MODE2D, 0.0, 0.0, 0.0, 0.0, 444, 444, 4));

    // Wait for 3 subscription messages to arrive
    ASSERT_TRUE(semaphore.tryAcquire(3, 20000));

    delete testProxyBuilder;
}

TEST_F(CombinedEnd2EndTest, subscribeToListAttribute) {

    //This is a workaround to register the Metatypes for providerQos.
    //Normally a new datatype is registered in all datatypes that use the new datatype.
    //However, when receiving a datatype as a returnValue of a RPC, the constructor has never been called before
    //so the datatype is not registered, and cannot be deserialized.
    qRegisterMetaType<types::ProviderQos>("types::ProviderQos");

    MockSubscriptionListenerOneType<QList<int> > *mockListener = new MockSubscriptionListenerOneType<QList<int> >();

    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener, onReceive(A<const QList<int>& >()))
            .WillRepeatedly(ReleaseSemaphore(&semaphore));

    QSharedPointer<ISubscriptionListener<QList<int> > > subscriptionListener(mockListener);
    // Provider: (runtime1)

    types::ProviderQos providerQos;
    providerQos.setPriority(2);
    std::shared_ptr<tests::testProvider> testProvider(new MockTestProvider(providerQos));
    std::string providerParticipantId = runtime1->registerProvider<tests::testProvider>(
            domainName,
            testProvider
    );

    //This wait is necessary, because registerProvider is async, and a lookup could occur
    // before the register has finished.
    QThreadSleep::msleep(5000);

    ProxyBuilder<tests::testProxy>* proxyBuilder
            = runtime2->createProxyBuilder<tests::testProxy>(domainName);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeout(1000);

    qlonglong qosRoundTripTTL = 40000;

    // Send a message and expect to get a result
    QSharedPointer<tests::testProxy> testProxy(proxyBuilder
                                               ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                               ->setCached(false)
                                               ->setDiscoveryQos(discoveryQos)
                                               ->build());

    QSharedPointer<SubscriptionQos> subscriptionQos(new OnChangeWithKeepAliveSubscriptionQos(
                                    500000,  // validity_ms
                                    1000,   // minInterval_ms
                                    2000,    // maxInterval_ms
                                    3000));   // alertInterval_ms
    QString subscriptionId = testProxy->subscribeToListOfInts(subscriptionListener, subscriptionQos);

    // Wait for 2 subscription messages to arrive
    ASSERT_TRUE(semaphore.tryAcquire(2, 20000));

    testProxy->unsubscribeFromListOfInts(subscriptionId);
    runtime1->unregisterProvider(providerParticipantId);
    delete proxyBuilder;
}

TEST_F(CombinedEnd2EndTest, subscribeToNonExistentDomain) {

	// Setup a mock listener - this will never be called
    qRegisterMetaType<types::ProviderQos>("types::ProviderQos");
    MockGpsSubscriptionListener* mockListener = new MockGpsSubscriptionListener();
	QSharedPointer<ISubscriptionListener<types::GpsLocation> > subscriptionListener(mockListener);

    std::string nonexistentDomain(std::string("non-existent-").append(uuid));

	// Create a proxy to a non-existent domain
    ProxyBuilder<tests::testProxy>* testProxyBuilder
            = runtime2->createProxyBuilder<tests::testProxy>(nonexistentDomain);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeout(1000);

    const int arbitrationTimeout = 5000;

    qlonglong qosRoundTripTTL = 40000;
    discoveryQos.setDiscoveryTimeout(arbitrationTimeout);

    // Time how long arbitration takes
    QTime timer;
    timer.start();
	bool haveArbitrationException = false;
    int elapsed = 0;

	// Expect an ArbitrationException
	try {
		// Send a message and expect to get a result
        QSharedPointer<tests::testProxy> testProxy(testProxyBuilder
                                                   ->setMessagingQos(MessagingQos(qosRoundTripTTL))
												   ->setCached(false)
                                                   ->setDiscoveryQos(discoveryQos)
												   ->build());
        QSharedPointer<SubscriptionQos> subscriptionQos(new OnChangeWithKeepAliveSubscriptionQos(
                                        500000,  // validity_ms
                                        1000,   // minInterval_ms
                                        2000,    //  maxInterval_ms
                                        3000));   // alertInterval_ms

        QString subscriptionId = testProxy->subscribeToLocation(subscriptionListener, subscriptionQos);

	} catch (JoynrArbitrationFailedException e) {
        haveArbitrationException = true;
        elapsed = timer.elapsed();
	}

	ASSERT_TRUE(haveArbitrationException);
    ASSERT_GE(elapsed, arbitrationTimeout);
    delete testProxyBuilder;
}


TEST_F(CombinedEnd2EndTest, unsubscribeViaHttpReceiver) {

    MockGpsSubscriptionListener* mockListener = new MockGpsSubscriptionListener();

    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener, onReceive(A<const types::GpsLocation&>()))
            .WillRepeatedly(ReleaseSemaphore(&semaphore));

    QSharedPointer<ISubscriptionListener<types::GpsLocation> > subscriptionListener(
                    mockListener);
    // Provider: (runtime1)

    types::ProviderQos providerQos;
    providerQos.setPriority(2);
    std::shared_ptr<tests::testProvider> testProvider(new tests::DefaulttestProvider(providerQos));
    //MockGpsProvider* gpsProvider = new MockGpsProvider();
    types::GpsLocation gpsLocation1;
    runtime1->registerProvider<tests::testProvider>(domainName, testProvider);

    //This wait is necessary, because registerProvider is async, and a lookup could occur
    // before the register has finished. See Joynr 805 for details
    QThreadSleep::msleep(5000);

    ProxyBuilder<tests::testProxy>* testProxyBuilder = runtime2->createProxyBuilder<tests::testProxy>(domainName);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeout(1000);

    qlonglong qosRoundTripTTL = 40000;

    // Send a message and expect to get a result
    QSharedPointer<tests::testProxy> gpsProxy(testProxyBuilder
                                               ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                               ->setCached(false)
                                               ->setDiscoveryQos(discoveryQos)
                                               ->build());
    QSharedPointer<SubscriptionQos> subscriptionQos(new OnChangeWithKeepAliveSubscriptionQos(
                                    9000,   // validity_ms
                                    1000,    // minInterval_ms
                                    2000,   //  maxInterval_ms
                                    10000));  // alertInterval_ms
    QString subscriptionId = gpsProxy->subscribeToLocation(subscriptionListener, subscriptionQos);

    // Wait for 2 subscription messages to arrive
    ASSERT_TRUE(semaphore.tryAcquire(2, 20000));

    gpsProxy->unsubscribeFromLocation(subscriptionId);

    // Check that the unsubscribe is eventually successful
    ASSERT_FALSE(semaphore.tryAcquire(4, 10000));

    delete testProxyBuilder;
}

TEST_F(CombinedEnd2EndTest, deleteChannelViaReceiver) {

    // Provider: (runtime1)

    types::ProviderQos providerQos;
    providerQos.setPriority(2);
    std::shared_ptr<tests::testProvider> testProvider(new tests::DefaulttestProvider(providerQos));
    //MockGpsProvider* gpsProvider = new MockGpsProvider();
    runtime1->registerProvider<tests::testProvider>(domainName, testProvider);

    QThreadSleep::msleep(1000); //This wait is necessary, because registerProvider is async, and a lookup could occour before the register has finished.

    ProxyBuilder<tests::testProxy>* testProxyBuilder = runtime2->createProxyBuilder<tests::testProxy>(domainName);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeout(1000);

    qlonglong qosRoundTripTTL = 40000;

    // Send a message and expect to get a result
    QSharedPointer<tests::testProxy> testProxy(testProxyBuilder
                                               ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                               ->setCached(false)
                                               ->setDiscoveryQos(discoveryQos)
                                               ->build());
    QThreadSleep::msleep(150);
    QSharedPointer<Future<int> > testFuture(testProxy->addNumbers(1, 2, 3));
    testFuture->waitForFinished();

    runtime1->deleteChannel();
    runtime2->deleteChannel();

    QSharedPointer<Future<int> > gpsFuture2(testProxy->addNumbers(1, 2, 3));
    gpsFuture2->waitForFinished(1000);

    delete testProxyBuilder;
}


TEST_F(CombinedEnd2EndTest, channelUrlProxyGetsNoUrlOnNonRegisteredChannel) {
    ProxyBuilder<infrastructure::ChannelUrlDirectoryProxy>* channelUrlDirectoryProxyBuilder =
            runtime1->createProxyBuilder<infrastructure::ChannelUrlDirectoryProxy>(
                TypeUtil::convertQStringtoStdString(messagingSettings1.getDiscoveryDirectoriesDomain())
            );

    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    infrastructure::ChannelUrlDirectoryProxy* channelUrlDirectoryProxy
            = channelUrlDirectoryProxyBuilder
                ->setMessagingQos(MessagingQos(1000))
                ->setCached(true)
                ->setDiscoveryQos(discoveryQos)
                ->build();

    RequestStatus status;
    types::ChannelUrlInformation result;
    QString channelId("test");
    channelUrlDirectoryProxy->getUrlsForChannel(status,result,channelId);
    EXPECT_EQ(status.getCode(), RequestStatusCode::ERROR_TIME_OUT_WAITING_FOR_RESPONSE);
}

TEST_F(CombinedEnd2EndTest, channelUrlProxyRegistersUrlsCorrectly) {
    ProxyBuilder<infrastructure::ChannelUrlDirectoryProxy>* channelUrlDirectoryProxyBuilder =
            runtime1->createProxyBuilder<infrastructure::ChannelUrlDirectoryProxy>(
                TypeUtil::convertQStringtoStdString(messagingSettings1.getDiscoveryDirectoriesDomain())
            );

    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    infrastructure::ChannelUrlDirectoryProxy* channelUrlDirectoryProxy
            = channelUrlDirectoryProxyBuilder
                ->setMessagingQos(MessagingQos(20000))
                ->setCached(true)
                ->setDiscoveryQos(discoveryQos)
                ->build();

    // There is a race condition where the actual channel url can be set AFTER the dummy data
    // used for testing. Pause for a short time so that the dummy data is always written
    // last
    QThreadSleep::msleep(2000);

    // Register new channel URLs
    RequestStatus status1;
    QString channelId = "bogus_1";
    types::ChannelUrlInformation channelUrlInformation;
    QList<QString> urls;
    urls << "bogusTestUrl_1" << "bogusTestUrl_2" ;
    channelUrlInformation.setUrls(urls);
    channelUrlDirectoryProxy->registerChannelUrls(
                status1,
                channelId,
                channelUrlInformation);

    EXPECT_TRUE(status1.successful()) << "Registering Url was not successful";
    RequestStatus status2;
    types::ChannelUrlInformation result;
    channelUrlDirectoryProxy->getUrlsForChannel(status2,result,channelId);
    EXPECT_TRUE(status2.successful())<< "Requesting Url was not successful";
    EXPECT_EQ(channelUrlInformation,result) << "Returned Url did not match Expected Url";
}



// This test is disabled, because the feature is not yet implemented on the server.
TEST_F(CombinedEnd2EndTest, DISABLED_channelUrlProxyUnRegistersUrlsCorrectly) {
    ProxyBuilder<infrastructure::ChannelUrlDirectoryProxy>* channelUrlDirectoryProxyBuilder =
            runtime1->createProxyBuilder<infrastructure::ChannelUrlDirectoryProxy>(
                TypeUtil::convertQStringtoStdString(messagingSettings1.getDiscoveryDirectoriesDomain())
            );

    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    infrastructure::ChannelUrlDirectoryProxy* channelUrlDirectoryProxy
            = channelUrlDirectoryProxyBuilder
                ->setMessagingQos(MessagingQos(10000))
                ->setCached(true)
                ->setDiscoveryQos(discoveryQos)
                ->build();

    RequestStatus status1;
    QString channelId = "bogus_3";
    types::ChannelUrlInformation channelUrlInformation;
    QList<QString> urls;
    urls << "bogusTestUrl_1" << "bogusTestUrl_2" ;
    channelUrlInformation.setUrls(urls);
    channelUrlDirectoryProxy->registerChannelUrls(status1, channelId, channelUrlInformation);

    EXPECT_TRUE(status1.successful());
    RequestStatus status2;
    types::ChannelUrlInformation result;
    channelUrlDirectoryProxy->getUrlsForChannel(status2,result,channelId);
    EXPECT_TRUE(status2.successful());
    EXPECT_EQ(channelUrlInformation,result);

    RequestStatus status3;
    channelUrlDirectoryProxy->unregisterChannelUrls(status3, channelId);
    EXPECT_TRUE(status3.successful());

    RequestStatus status4;
    types::ChannelUrlInformation result2;
    channelUrlDirectoryProxy->getUrlsForChannel(status4,result2,channelId);
    EXPECT_EQ(0,result2.getUrls().size());
    EXPECT_FALSE(status4.successful());
}

tests::testProxy* createTestProxy(JoynrClusterControllerRuntime *runtime, const std::string& domainName){
    ProxyBuilder<tests::testProxy>* testProxyBuilder
           = runtime->createProxyBuilder<tests::testProxy>(domainName);
   DiscoveryQos discoveryQos;
   discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
   discoveryQos.setDiscoveryTimeout(1000);

   qlonglong qosRoundTripTTL = 40000;

   // Send a message and expect to get a result
   tests::testProxy* testProxy(testProxyBuilder
      ->setMessagingQos(MessagingQos(qosRoundTripTTL))
      ->setCached(false)
      ->setDiscoveryQos(discoveryQos)
      ->build());
   delete testProxyBuilder;
   return testProxy;
}


// A function that subscribes to a GpsPosition - to be run in a background thread
void subscribeToLocation(QSharedPointer<ISubscriptionListener<types::GpsLocation> > listener,
                            tests::testProxy* testProxy,
                            CombinedEnd2EndTest* testSuite) {
    QSharedPointer<SubscriptionQos> subscriptionQos(new OnChangeWithKeepAliveSubscriptionQos(
                                    500000,   // validity_ms
                                    1000,    // minInterval_ms
                                    2000,   //  maxInterval_ms
                                    3000));  // alertInterval_ms
    testSuite->registeredSubscriptionId = testProxy->subscribeToLocation(listener, subscriptionQos);
}

// A function that subscribes to a GpsPosition - to be run in a background thread
void unsubscribeFromLocation(tests::testProxy* testProxy,
                            QString subscriptionId) {
    testProxy->unsubscribeFromLocation(subscriptionId);
}


// This test was written to model a bug report where a subscription started in a background thread
// causes a runtime error to be reported by Qt
TEST_F(CombinedEnd2EndTest, subscribeInBackgroundThread) {

    qRegisterMetaType<types::ProviderQos>("types::ProviderQos");

    MockGpsSubscriptionListener* mockListener = new MockGpsSubscriptionListener();

    // Use a semaphore to count and wait on calls to the mock listener
    // QSemaphore semaphore(0);
    EXPECT_CALL(*mockListener, onReceive(A<const types::GpsLocation&>()))
            .WillRepeatedly(ReleaseSemaphore(&semaphore));

    QSharedPointer<ISubscriptionListener<types::GpsLocation> > subscriptionListener(
                    mockListener);

    types::ProviderQos providerQos;
    providerQos.setPriority(2);
    std::shared_ptr<tests::testProvider> testProvider(new tests::DefaulttestProvider(providerQos));
    std::string providerParticipantId = runtime1->registerProvider<tests::testProvider>(
            domainName,
            testProvider
    );

    //This wait is necessary, because registerProvider is async, and a lookup could occur
    // before the register has finished.
    QThreadSleep::msleep(5000);

    tests::testProxy* testProxy = createTestProxy(runtime2, domainName);
    // Subscribe in a background thread
    QtConcurrent::run(subscribeToLocation, subscriptionListener, testProxy, this);

    // Wait for 2 subscription messages to arrive
    ASSERT_TRUE(semaphore.tryAcquire(2, 20000));

    unsubscribeFromLocation(testProxy, registeredSubscriptionId);

    runtime1->unregisterProvider(providerParticipantId);
}

TEST_F(CombinedEnd2EndTest, call_async_void_operation) {
    types::ProviderQos providerQos;
    providerQos.setPriority(2);
    std::shared_ptr<tests::testProvider> testProvider(new MockTestProvider(providerQos));

    QThreadSleep::msleep(100);

    runtime1->registerProvider<tests::testProvider>(domainName, testProvider);

    QThreadSleep::msleep(100);

    ProxyBuilder<tests::testProxy>* testProxyBuilder = runtime2->createProxyBuilder<tests::testProxy>(domainName);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeout(1000);

    qlonglong qosRoundTripTTL = 20000;

    // Send a message and expect to get a result
    QSharedPointer<tests::testProxy> testProxy(testProxyBuilder
                                                   ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                                   ->setCached(false)
                                                   ->setDiscoveryQos(discoveryQos)
                                                   ->build());

    // Setup a callbackFct
    std::function<void(const joynr::RequestStatus& status)> callbackFct = [] (const joynr::RequestStatus& status){
       ASSERT_TRUE(status.successful());
    };

    // Asynchonously call the void operation
    QSharedPointer<Future<void> > future (testProxy->voidOperation(callbackFct));

    // Wait for the operation to finish and check for a successful callback
    future->waitForFinished();
    ASSERT_TRUE(future->getStatus().successful());

    delete testProxyBuilder;
}

TEST_F(CombinedEnd2EndTest, call_async_void_operation_failure) {
    types::ProviderQos providerQos;
    providerQos.setPriority(2);
    std::shared_ptr<tests::testProvider> testProvider(new MockTestProvider(providerQos));

    QThreadSleep::msleep(2550);

    std::string testProviderParticipantId = runtime1->registerProvider<tests::testProvider>(
            domainName,
            testProvider
    );

    QThreadSleep::msleep(2550);

    ProxyBuilder<tests::testProxy>* testProxyBuilder = runtime2->createProxyBuilder<tests::testProxy>(domainName);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeout(1000);

    qlonglong qosRoundTripTTL = 20000;

    // Send a message and expect to get a result
    QSharedPointer<tests::testProxy> testProxy(testProxyBuilder
                                                   ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                                   ->setCached(false)
                                                   ->setDiscoveryQos(discoveryQos)
                                                   ->build());

    // Shut down the provider
    runtime1->stopMessaging();
    QThreadSleep::msleep(5000);

    // Setup a callbackFct
    std::function<void(const joynr::RequestStatus&)> callbackFct = [] (const joynr::RequestStatus& status) {
        ASSERT_FALSE(status.successful());
    };

    // Asynchonously call the void operation
    QSharedPointer<Future<void> > future (testProxy->voidOperation(callbackFct));

    // Wait for the operation to finish and check for a failure callback
    future->waitForFinished();
    ASSERT_FALSE(future->getStatus().successful());

    runtime1->unregisterProvider(testProviderParticipantId);

    delete testProxyBuilder;
}
