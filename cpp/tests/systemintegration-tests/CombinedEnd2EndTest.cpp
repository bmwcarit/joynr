/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
#include <cstdint>
#include <chrono>
#include <QtConcurrent/QtConcurrent>

#include "systemintegration-tests/CombinedEnd2EndTest.h"
#include "systemintegration-tests/TestConfiguration.h"
#include "tests/utils/LibJoynrMockObjects.h"
#include "joynr/JoynrRuntime.h"
#include "joynr/LibjoynrSettings.h"
#include "joynr/tests/testProxy.h"
#include "joynr/tests/testTypes/DerivedStruct.h"
#include "joynr/tests/testTypes/AnotherDerivedStruct.h"
#include "joynr/types/Localisation/Trip.h"
#include "joynr/types/Localisation/GpsLocation.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/types/CapabilityInformation.h"
#include "joynr/CapabilitiesRegistrar.h"
#include "PrettyPrint.h"
#include "joynr/MessagingSettings.h"
#include "joynr/Future.h"
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"
#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/Logger.h"

using namespace ::testing;
using namespace joynr;

ACTION_P(ReleaseSemaphore,semaphore)
{
    semaphore->notify();
}

static const std::string messagingPropertiesPersistenceFileName1("CombinedEnd2EndTest-runtime1-joynr.settings");
static const std::string messagingPropertiesPersistenceFileName2("CombinedEnd2EndTest-runtime2-joynr.settings");

INIT_LOGGER(CombinedEnd2EndTest);

CombinedEnd2EndTest::CombinedEnd2EndTest() :
        runtime1(nullptr),
        runtime2(nullptr),
        settings1("test-resources/SystemIntegrationTest1.settings"),
        settings2("test-resources/SystemIntegrationTest2.settings"),
        messagingSettings1(settings1),
        messagingSettings2(settings2),
        baseUuid(Util::createUuid()),
        uuid( "_" + baseUuid.substr(1, baseUuid.length()-2)),
        domainName("cppCombinedEnd2EndTest_Domain" + uuid),
        semaphore(0)
{
    messagingSettings1.setMessagingPropertiesPersistenceFilename(messagingPropertiesPersistenceFileName1);
    messagingSettings2.setMessagingPropertiesPersistenceFilename(messagingPropertiesPersistenceFileName2);
}

void CombinedEnd2EndTest::SetUp()
{
    JOYNR_LOG_DEBUG(logger, "SetUp() CombinedEnd2End");

    // See if the test environment has overridden the configuration files
    tests::Configuration& configuration = tests::Configuration::getInstance();
    std::string systemSettingsFile = configuration.getDefaultSystemSettingsFile();
    std::string websocketSettingsFile = configuration.getDefaultWebsocketSettingsFile();

    JOYNR_LOG_DEBUG(logger, "Default system settings file: {}",systemSettingsFile.c_str());
    JOYNR_LOG_DEBUG(logger, "Default websocket settings file: {}",websocketSettingsFile.c_str());

    if (systemSettingsFile.empty() && websocketSettingsFile.empty()) {
        runtime1 = JoynrRuntime::createRuntime("test-resources/libjoynrSystemIntegration1.settings",
                                               "test-resources/SystemIntegrationTest1.settings");
        runtime2= JoynrRuntime::createRuntime("test-resources/libjoynrSystemIntegration2.settings",
                                              "test-resources/SystemIntegrationTest2.settings");
    } else {
        runtime1 = JoynrRuntime::createRuntime(systemSettingsFile, websocketSettingsFile);
        runtime2 = JoynrRuntime::createRuntime(systemSettingsFile, websocketSettingsFile);
    }
}

void CombinedEnd2EndTest::TearDown()
{
    // Cause a shutdown by deleting the runtimes
    delete runtime1;
    delete runtime2;

    // Delete the persisted participant ids so that each test uses different participant ids
    std::remove(LibjoynrSettings::DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME().c_str());
}

CombinedEnd2EndTest::~CombinedEnd2EndTest()
{
}

TEST_F(CombinedEnd2EndTest, callRpcMethodViaHttpReceiverAndReceiveReply) {

    // Provider: (runtime1)
    types::ProviderQos providerQos;
    providerQos.setPriority(2);
    std::shared_ptr<tests::testProvider> testProvider(new MockTestProvider(providerQos));

    std::this_thread::sleep_for(std::chrono::seconds(1));

    runtime1->registerProvider<tests::testProvider>(domainName, testProvider);

    std::this_thread::sleep_for(std::chrono::seconds(1));

    //consumer for testinterface
    // Testing Lists
    {
        ProxyBuilder<tests::testProxy>* testProxyBuilder = runtime2->createProxyBuilder<tests::testProxy>(domainName);
        DiscoveryQos discoveryQos;
        discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
        discoveryQos.setDiscoveryTimeout(1000);

        std::uint64_t qosRoundTripTTL = 40000;

        // Send a message and expect to get a result
        std::shared_ptr<tests::testProxy> testProxy(testProxyBuilder
                                                   ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                                   ->setCached(false)
                                                   ->setDiscoveryQos(discoveryQos)
                                                   ->build());

        std::vector<int> list;
        list.push_back(2);
        list.push_back(4);
        list.push_back(8);
        std::shared_ptr<Future<int> >gpsFuture (testProxy->sumIntsAsync(list));
        gpsFuture->wait();
        int expectedValue = 2+4+8;
        ASSERT_TRUE(gpsFuture->getStatus().successful());
        int actualValue;
        gpsFuture->get(actualValue);
        EXPECT_EQ(expectedValue, actualValue);
        //TODO CA: shared pointer for proxy builder?

     /*
      * Testing TRIP
      * Now try to send a Trip (which contains a list) and check if the returned trip is identical.
      */

        std::vector<types::Localisation::GpsLocation> inputLocationList;
        inputLocationList.push_back(types::Localisation::GpsLocation(1.1, 2.2, 3.3, types::Localisation::GpsFixEnum::MODE2D, 0.0, 0.0, 0.0, 0.0, 444, 444, 4));
        inputLocationList.push_back(types::Localisation::GpsLocation(1.1, 2.2, 3.3, types::Localisation::GpsFixEnum::MODE2D, 0.0, 0.0, 0.0, 0.0, 444, 444, 5));
        inputLocationList.push_back(types::Localisation::GpsLocation(1.1, 2.2, 3.3, types::Localisation::GpsFixEnum::MODE2D, 0.0, 0.0, 0.0, 0.0, 444, 444, 6));
        types::Localisation::Trip inputTrip;
        inputTrip.setLocations(inputLocationList);
        std::shared_ptr<Future<types::Localisation::Trip> > tripFuture (testProxy->optimizeTripAsync(inputTrip));
        tripFuture->wait();
        ASSERT_EQ(RequestStatusCode::OK, tripFuture->getStatus().getCode());
        types::Localisation::Trip actualTrip;
        tripFuture->get(actualTrip);
        EXPECT_EQ(inputTrip, actualTrip);

     /*
      * Testing Lists in returnvalues
      * Now try to send call a method that has a list as return parameter
      */

        std::vector<int> primesBelow6;
        primesBelow6.push_back(2);
        primesBelow6.push_back(3);
        primesBelow6.push_back(5);
        std::vector<int> result;
        try {
            testProxy->returnPrimeNumbers(result, 6);
            EXPECT_EQ(primesBelow6, result);
        } catch (exceptions::JoynrException& e) {
            FAIL()<< "returnPrimeNumbers was not successful";
        }

        delete testProxyBuilder;

    /*
     * Testing List of Vowels
     * Now try to send a List of Vowels and see if it is returned correctly.
     */
// does not compile, see 654
//       std::vector<Vowel> inputWord;
//       inputWord.push_back("h");
//       inputWord.push_back("e");
//       inputWord.push_back("l");
//       inputWord.push_back("l");
//       inputWord.push_back("o");
//       std::shared_ptr<Future<std::vector<Vowel> > > wordFuture (new std::shared_ptr<Future<std::vector<Vowel> > >());
//       testProxy->optimizeWord(wordFuture, inputTrip);
//       wordFuture->wait();
//       ASSERT_EQ(RequestStatusCode::OK, wordFuture->getStatus().getCode());
//       inputTrip.push_back("a"); //thats what optimize word does.. appending an a.
//       EXPECT_EQ(inputTrip, tripFuture->getValue());

    /*
     * Testing List of Locations
     * Now try to send a List of GpsLocations and see if it is returned correctly.
     */
        // is currently deactivated, because it throws an assertion.
        std::vector<types::Localisation::GpsLocation> inputGpsLocationList;
        inputGpsLocationList.push_back(types::Localisation::GpsLocation(1.1, 2.2, 3.3, types::Localisation::GpsFixEnum::MODE2D, 0.0, 0.0, 0.0, 0.0, 444, 444, 4));
        inputGpsLocationList.push_back(types::Localisation::GpsLocation(1.1, 2.2, 3.3, types::Localisation::GpsFixEnum::MODE2D, 0.0, 0.0, 0.0, 0.0, 444, 444, 5));
        inputGpsLocationList.push_back(types::Localisation::GpsLocation(1.1, 2.2, 3.3, types::Localisation::GpsFixEnum::MODE2D, 0.0, 0.0, 0.0, 0.0, 444, 444, 6));
        std::shared_ptr<Future<std::vector<types::Localisation::GpsLocation> > > listLocationFuture (testProxy->optimizeLocationListAsync(inputGpsLocationList));
        listLocationFuture->wait();
        ASSERT_EQ(RequestStatusCode::OK, listLocationFuture->getStatus().getCode());
        std::vector<joynr::types::Localisation::GpsLocation> actualLocation;
        listLocationFuture->get(actualLocation);
        EXPECT_EQ(inputGpsLocationList, actualLocation);

     /*
      * Testing GetAttribute, when setAttribute has been called locally.
      *
      */
        // Primes
        int testPrimeValue = 15;

        std::function<void()> onSuccess = [] () {};

        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError = [] (const joynr::exceptions::ProviderRuntimeException& exception) {};

        /*
         * because of the implementation of the MockTestProvider,
         * we can use the async API of the testProvider in a sync way
         */
        testProvider->setFirstPrime(testPrimeValue, onSuccess, onError);

        int primeResult(0);
        try {
            testProxy->getFirstPrime(primeResult);
        } catch (exceptions::JoynrException& e) {
            FAIL()<< "getFirstPrime was not successful";
        }
        EXPECT_EQ(primeResult, 15);

        // List of strings,
        std::vector<std::string> localStrList;
        std::vector<std::string> remoteStrList;
        localStrList.push_back("one ü");
        localStrList.push_back("two 漢語");
        localStrList.push_back("three ـتـ");
        localStrList.push_back("four {");
        testProvider->setListOfStrings(localStrList, onSuccess, onError);

        try {
            testProxy->getListOfStrings(remoteStrList);
        } catch (exceptions::JoynrException& e) {
            FAIL()<< "getListOfStrings was not successful";
        }
        EXPECT_EQ(localStrList, remoteStrList);

     /*
      * Testing GetAttribute with Remote SetAttribute
      *
      */

        try {
            testProxy->setFirstPrime(19);
        } catch (exceptions::JoynrException& e) {
            FAIL()<< "setFirstPrime was not successful";
        }
        try {
            testProxy->getFirstPrime(primeResult);
        } catch (exceptions::JoynrException& e) {
            FAIL()<< "getFirstPrime was not successful";
        }
        EXPECT_EQ(primeResult, 19);

      /*
        *
        * Testing local/remote getters and setters with lists.
        */

        std::vector<int> inputIntList;
        inputIntList.push_back(2);
        inputIntList.push_back(3);
        inputIntList.push_back(5);
        testProvider->setListOfInts(inputIntList, onSuccess, onError);
        std::vector<int> outputIntLIst;
        try {
            testProxy->getListOfInts(outputIntLIst);
        } catch (exceptions::JoynrException& e) {
            FAIL()<< "getListOfInts was not successful";
        }
        EXPECT_EQ(outputIntLIst, inputIntList);
        EXPECT_EQ(outputIntLIst.at(1), 3);
        //test remote setter
        inputIntList.clear();
        inputIntList.push_back(7);
        inputIntList.push_back(11);
        inputIntList.push_back(13);
        try {
            testProxy->setListOfInts(inputIntList);
        } catch (exceptions::JoynrException& e) {
            FAIL()<< "setListOfInts was not successful";
        }
        try {
            testProxy->getListOfInts(outputIntLIst);
        } catch (exceptions::JoynrException& e) {
            FAIL()<< "getListOfInts was not successful";
        }
        EXPECT_EQ(outputIntLIst, inputIntList);
        EXPECT_EQ(outputIntLIst.at(1), 11);

        // Testing enums
        testProxy->setEnumAttribute(tests::testTypes::TestEnum::TWO);
        tests::testTypes::TestEnum::Enum actualEnumAttribute;
        testProxy->getEnumAttribute(actualEnumAttribute);
        EXPECT_EQ(actualEnumAttribute, tests::testTypes::TestEnum::TWO);
        try {
            testProxy->setEnumAttribute(static_cast<tests::testTypes::TestEnum::Enum>(999));
            ASSERT_FALSE(true) << "This line of code should never be reached";
        } catch (joynr::exceptions::MethodInvocationException e) {
            JOYNR_LOG_DEBUG(logger, "Expected joynr::exceptions::MethodInvocationException has been thrown. Message: {}",e.getMessage());
        } catch (std::exception e) {
            ASSERT_FALSE(true) << "joynr::exceptions::MethodInvocationException is expected, however exception with message " << e.what() << "is thrown";
        }

        // Testing byte buffer
        joynr::ByteBuffer byteBufferValue {1,2,3};
        testProxy->setByteBufferAttribute(byteBufferValue);
        joynr::ByteBuffer actualByteBufferValue;
        testProxy->getByteBufferAttribute(actualByteBufferValue);
        EXPECT_EQ(actualByteBufferValue, byteBufferValue);

        joynr::ByteBuffer returnByteBufferValue;
        testProxy->methodWithByteBuffer(returnByteBufferValue, byteBufferValue);
        EXPECT_EQ(returnByteBufferValue, byteBufferValue);
    }

    // Testing TTL
    {
       //create a proxy with very short TTL and expect no returning replies.
        ProxyBuilder<tests::testProxy>* testProxyBuilder = runtime2->createProxyBuilder<tests::testProxy>(domainName);
        DiscoveryQos discoveryQos;
        discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
        discoveryQos.setDiscoveryTimeout(1000);

        std::uint64_t qosRoundTripTTL = 1;
        std::shared_ptr<tests::testProxy> testProxy(testProxyBuilder
                                                   ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                                   ->setCached(false)
                                                   ->setDiscoveryQos(discoveryQos)
                                                   ->build());
        std::shared_ptr<Future<int> > testFuture(testProxy->addNumbersAsync(1, 2, 3));
        testFuture->wait();
        ASSERT_EQ(testFuture->getStatus().getCode(), RequestStatusCode::ERROR_TIMEOUT_WAITING_FOR_RESPONSE);
        //TODO CA: shared pointer for proxy builder?
        delete testProxyBuilder;
    }

    // TESTING Attribute getter of an array of a nested struct
    {
        ProxyBuilder<tests::testProxy>* testProxyBuilder = runtime2->createProxyBuilder<tests::testProxy>(domainName);
        DiscoveryQos discoveryQos;
        discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
        discoveryQos.setDiscoveryTimeout(1000);

        std::uint64_t qosRoundTripTTL = 40000;

        // Send a message and expect to get a result
        std::shared_ptr<tests::testProxy> testProxy(testProxyBuilder
                                                   ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                                   ->setCached(false)
                                                   ->setDiscoveryQos(discoveryQos)
                                                   ->build());
        std::vector<joynr::tests::testTypes::HavingComplexArrayMemberStruct> setValue;
        std::vector<joynr::tests::testTypes::NeverUsedAsAttributeTypeOrMethodParameterStruct> arrayMember;
        arrayMember.push_back(joynr::tests::testTypes::NeverUsedAsAttributeTypeOrMethodParameterStruct("neverUsed"));
        setValue.push_back(joynr::tests::testTypes::HavingComplexArrayMemberStruct(arrayMember));
        testProxy->setAttributeArrayOfNestedStructs(setValue);

        std::vector<joynr::tests::testTypes::HavingComplexArrayMemberStruct> result;
        testProxy->getAttributeArrayOfNestedStructs(result);
        ASSERT_EQ(result, setValue);
    }

    // TESTING getter/setter and operation calls with different kinds of parameters (maps, complex structs, ...)
    {
        using namespace joynr::types::TestTypes;
        ProxyBuilder<tests::testProxy>* testProxyBuilder = runtime2->createProxyBuilder<tests::testProxy>(domainName);
        DiscoveryQos discoveryQos;
        discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
        discoveryQos.setDiscoveryTimeout(1000);

        std::uint64_t qosRoundTripTTL = 40000;

        // Send a message and expect to get a result
        std::shared_ptr<tests::testProxy> testProxy(testProxyBuilder
                                                   ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                                   ->setCached(false)
                                                   ->setDiscoveryQos(discoveryQos)
                                                   ->build());

        TEverythingMap setValue;
        setValue.insert({TEnum::TLITERALA, TEverythingExtendedStruct()});
        setValue.insert({TEnum::TLITERALB, TEverythingExtendedStruct()});
        testProxy->setEverythingMap(setValue);

        TEverythingMap result;
        testProxy->getEverythingMap(result);
        ASSERT_EQ(result, setValue);

        TStringKeyMap mapParameterResult;
        TStringKeyMap stringKeyMap;
        stringKeyMap.insert({"StringKey1", "StringValue1"});
        stringKeyMap.insert({"StringKey2", "StringValue2"});

        testProxy->mapParameters(mapParameterResult, stringKeyMap);
        ASSERT_EQ(mapParameterResult, stringKeyMap);

        bool booleanOut;
        double doubleOut;
        float floatOut;
        std::int8_t int8Out;
        std::int16_t int16Out;
        std::int32_t int32Out;
        std::int64_t int64Out;
        std::uint8_t uint8Out;
        std::uint16_t uint16Out;
        std::uint32_t uint32Out;
        std::uint64_t uint64Out;
        std::string stringOut;

        bool booleanArg = true;
        double doubleArg = 1.1;
        float floatArg = 2.2;
        std::int8_t int8Arg = 6;
        std::int16_t int16Arg = 3;
        std::int32_t int32Arg = 4;
        std::int64_t int64Arg = 5;
        std::string stringArg = "7";
        std::uint16_t uint16Arg = 8;
        std::uint32_t uint32Arg = 9;
        std::uint64_t uint64Arg = 10;
        std::uint8_t uint8Arg = 11;
        testProxy->methodWithAllPossiblePrimitiveParameters(booleanOut,
                                                            doubleOut,
                                                            floatOut,
                                                            int16Out,
                                                            int32Out,
                                                            int64Out,
                                                            int8Out,
                                                            stringOut,
                                                            uint16Out,
                                                            uint32Out,
                                                            uint64Out,
                                                            uint8Out,
                                                            booleanArg,
                                                            doubleArg,
                                                            floatArg,
                                                            int16Arg,
                                                            int32Arg,
                                                            int64Arg,
                                                            int8Arg,
                                                            stringArg,
                                                            uint16Arg,
                                                            uint32Arg,
                                                            uint64Arg,
                                                            uint8Arg);

        EXPECT_EQ(booleanOut, booleanArg);
        EXPECT_DOUBLE_EQ(doubleOut, doubleArg);
        EXPECT_FLOAT_EQ(floatOut, floatArg);
        EXPECT_EQ(stringOut, stringArg);
        EXPECT_EQ(int8Out, int8Arg);
        EXPECT_EQ(int16Out, int16Arg);
        EXPECT_EQ(int32Out, int32Arg);
        EXPECT_EQ(int64Out, int64Arg);
        EXPECT_EQ(uint8Out, uint8Arg);
        EXPECT_EQ(uint16Out, uint16Arg);
        EXPECT_EQ(uint32Out, uint32Arg);
        EXPECT_EQ(uint64Out, uint64Arg);
    }

    // Operation overloading is not currently supported
#if 0
    // Testing operation overloading
    {
        ProxyBuilder<tests::TestProxy>* testProxyBuilder = runtime2->createProxyBuilder<tests::testProxy>(domainName);
        DiscoveryQos discoveryQos;
        discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HighestPriority);
        discoveryQos.setDiscoveryTimeout(1000);

        std::uint64_t qosOneWayTTL = 40000;
        std::uint64_t qosRoundTripTTL = 40000;

        // Send a message and expect to get a result
        std::shared_ptr<tests::testProxy> testProxy(testProxyBuilder
                                                   ->setMessagingQos(MessagingQos(qosOneWayTTL, qosRoundTripTTL))
                                                   ->setCached(false)
                                                   ->setDiscoveryQos(discoveryQos)
                                                   ->build());

        std::string derivedStructResult;
        std::string anotherDerivedStructResult;

        // Check that the operation overloading worked and the result is of the correct type
        testProxy->overloadedOperation(derivedStructResult, tests::DerivedStruct());
        testProxy->overloadedOperation(anotherDerivedStructResult, tests::AnotherDerivedStruct());
        EXPECT_EQ(derivedStructResult, "DerivedStruct");
        EXPECT_EQ(anotherDerivedStructResult, "AnotherDerivedStruct");
    }
#endif

}


TEST_F(CombinedEnd2EndTest, subscribeViaHttpReceiverAndReceiveReply) {

    MockGpsSubscriptionListener* mockListener = new MockGpsSubscriptionListener();

    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener, onReceive(A<const types::Localisation::GpsLocation&>()))
            .WillRepeatedly(ReleaseSemaphore(&semaphore));

    std::shared_ptr<ISubscriptionListener<types::Localisation::GpsLocation> > subscriptionListener(
                    mockListener);
    // Provider: (runtime1)

    std::shared_ptr<tests::testProvider> testProvider(new tests::DefaulttestProvider());
    //MockGpsProvider* gpsProvider = new MockGpsProvider();
    runtime1->registerProvider<tests::testProvider>(domainName, testProvider);

    //This wait is necessary, because registerProvider is async, and a lookup could occur
    // before the register has finished.
    std::this_thread::sleep_for(std::chrono::seconds(5));

    ProxyBuilder<tests::testProxy>* testProxyBuilder
            = runtime2->createProxyBuilder<tests::testProxy>(domainName);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeout(1000);

    std::uint64_t qosRoundTripTTL = 40000;

    // Send a message and expect to get a result
    std::shared_ptr<tests::testProxy> testProxy(testProxyBuilder
                                               ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                               ->setCached(false)
                                               ->setDiscoveryQos(discoveryQos)
                                               ->build());

    std::int64_t minInterval_ms = 1000;
    std::int64_t maxInterval_ms = 2000;

    OnChangeWithKeepAliveSubscriptionQos subscriptionQos(
                                    10000,   // validity_ms
                                    minInterval_ms,
                                    maxInterval_ms,
                                    3000);  // alertInterval_ms
    std::string subscriptionId = testProxy->subscribeToLocation(subscriptionListener, subscriptionQos);

    // Wait for 2 subscription messages to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(20)));
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(20)));

    testProxy->unsubscribeFromLocation(subscriptionId);

    delete testProxyBuilder;
}

TEST_F(CombinedEnd2EndTest, subscribeToOnChange) {
    MockGpsSubscriptionListener* mockListener = new MockGpsSubscriptionListener();

    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener, onReceive(A<const types::Localisation::GpsLocation&>()))
            .WillRepeatedly(ReleaseSemaphore(&semaphore));

    std::shared_ptr<ISubscriptionListener<types::Localisation::GpsLocation> > subscriptionListener(
                    mockListener);
    // Provider: (runtime1)

    std::shared_ptr<tests::testProvider> testProvider(new tests::DefaulttestProvider());
    runtime1->registerProvider<tests::testProvider>(domainName, testProvider);

    //This wait is necessary, because registerProvider is async, and a lookup could occur
    // before the register has finished.
    std::this_thread::sleep_for(std::chrono::seconds(5));

    ProxyBuilder<tests::testProxy>* testProxyBuilder
            = runtime2->createProxyBuilder<tests::testProxy>(domainName);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeout(1000);

    std::uint64_t qosRoundTripTTL = 40000;

    // Send a message and expect to get a result
    std::shared_ptr<tests::testProxy> testProxy(testProxyBuilder
                                               ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                               ->setCached(false)
                                               ->setDiscoveryQos(discoveryQos)
                                               ->build());

    // Publications will be sent maintaining this minimum interval provided, even if the value
    // changes more often. This prevents the consumer from being flooded by updated values.
    // The filtering happens on the provider's side, thus also preventing excessive network traffic.
    // This value is provided in milliseconds. The minimum value for minInterval is 50 ms.
    std::int64_t minInterval_ms = 50;
    OnChangeSubscriptionQos subscriptionQos(
                                    500000,   // validity_ms
                                    minInterval_ms);  // minInterval_ms
    std::string subscriptionId = testProxy->subscribeToLocation(subscriptionListener, subscriptionQos);

    //This wait is necessary, because subcriptions are async, and an attribute could be changed before
    // before the subscription has started.
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Change the location once
    testProxy->setLocation(types::Localisation::GpsLocation(9.0, 51.0, 508.0, types::Localisation::GpsFixEnum::MODE2D, 0.0, 0.0, 0.0, 0.0, 444, 444, 1));

    // Wait for a subscription message to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(20)));

    // Change the location 3 times
    testProxy->setLocation(types::Localisation::GpsLocation(9.0, 51.0, 508.0, types::Localisation::GpsFixEnum::MODE2D, 0.0, 0.0, 0.0, 0.0, 444, 444, 2));
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms + 50));
    testProxy->setLocation(types::Localisation::GpsLocation(9.0, 51.0, 508.0, types::Localisation::GpsFixEnum::MODE2D, 0.0, 0.0, 0.0, 0.0, 444, 444, 3));
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms + 50));
    testProxy->setLocation(types::Localisation::GpsLocation(9.0, 51.0, 508.0, types::Localisation::GpsFixEnum::MODE2D, 0.0, 0.0, 0.0, 0.0, 444, 444, 4));

    // Wait for 3 subscription messages to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(20)));
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(20)));
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(20)));

    delete testProxyBuilder;
}

TEST_F(CombinedEnd2EndTest, subscribeToListAttribute) {

    MockSubscriptionListenerOneType<std::vector<int>> *mockListener = new MockSubscriptionListenerOneType<std::vector<int>>();

    std::vector<int> expectedValues = {1000, 2000, 3000};
    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener, onReceive(Eq(expectedValues)))
            .WillRepeatedly(ReleaseSemaphore(&semaphore));

    std::shared_ptr<ISubscriptionListener<std::vector<int>>> subscriptionListener(mockListener);
    // Provider: (runtime1)

    types::ProviderQos providerQos;
    providerQos.setPriority(2);
    std::shared_ptr<tests::testProvider> testProvider(new MockTestProvider(providerQos));
    testProvider->setListOfInts(expectedValues, [](){}, [](const joynr::exceptions::JoynrRuntimeException&){});
    std::string providerParticipantId = runtime1->registerProvider<tests::testProvider>(
            domainName,
            testProvider
    );

    //This wait is necessary, because registerProvider is async, and a lookup could occur
    // before the register has finished.
    std::this_thread::sleep_for(std::chrono::seconds(5));

    ProxyBuilder<tests::testProxy>* proxyBuilder
            = runtime2->createProxyBuilder<tests::testProxy>(domainName);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeout(1000);

    std::uint64_t qosRoundTripTTL = 40000;

    // Send a message and expect to get a result
    std::shared_ptr<tests::testProxy> testProxy(proxyBuilder
                                               ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                               ->setCached(false)
                                               ->setDiscoveryQos(discoveryQos)
                                               ->build());

    OnChangeWithKeepAliveSubscriptionQos subscriptionQos(
                                    500000,  // validity_ms
                                    1000,   // minInterval_ms
                                    2000,    // maxInterval_ms
                                    3000);   // alertInterval_ms
    std::string subscriptionId = testProxy->subscribeToListOfInts(subscriptionListener, subscriptionQos);

    // Wait for 2 subscription messages to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(20)));
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(20)));

    testProxy->unsubscribeFromListOfInts(subscriptionId);
    runtime1->unregisterProvider(providerParticipantId);
    delete proxyBuilder;
}

TEST_F(CombinedEnd2EndTest, subscribeToNonExistentDomain) {

	// Setup a mock listener - this will never be called
    MockGpsSubscriptionListener* mockListener = new MockGpsSubscriptionListener();
    std::shared_ptr<ISubscriptionListener<types::Localisation::GpsLocation> > subscriptionListener(mockListener);

    std::string nonexistentDomain(std::string("non-existent-").append(uuid));

	// Create a proxy to a non-existent domain
    ProxyBuilder<tests::testProxy>* testProxyBuilder
            = runtime2->createProxyBuilder<tests::testProxy>(nonexistentDomain);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeout(1000);

    const int arbitrationTimeout = 5000;

    std::uint64_t qosRoundTripTTL = 40000;
    discoveryQos.setDiscoveryTimeout(arbitrationTimeout);

    // Time how long arbitration takes
    auto start = std::chrono::system_clock::now();
    bool haveDiscoveryException = false;
    int elapsed = 0;

	// Expect an ArbitrationException
	try {
		// Send a message and expect to get a result
        std::shared_ptr<tests::testProxy> testProxy(testProxyBuilder
                                                   ->setMessagingQos(MessagingQos(qosRoundTripTTL))
												   ->setCached(false)
                                                   ->setDiscoveryQos(discoveryQos)
												   ->build());
        OnChangeWithKeepAliveSubscriptionQos subscriptionQos(
                                        500000,  // validity_ms
                                        1000,   // minInterval_ms
                                        2000,    //  maxInterval_ms
                                        3000);   // alertInterval_ms

        std::string subscriptionId = testProxy->subscribeToLocation(subscriptionListener, subscriptionQos);

	} catch (exceptions::DiscoveryException& e) {
        haveDiscoveryException = true;
        auto now = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
        elapsed = duration.count();
	}

	ASSERT_TRUE(haveDiscoveryException);
    ASSERT_GE(elapsed, arbitrationTimeout);
    delete testProxyBuilder;
}


TEST_F(CombinedEnd2EndTest, unsubscribeViaHttpReceiver) {

    MockGpsSubscriptionListener* mockListener = new MockGpsSubscriptionListener();

    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener, onReceive(A<const types::Localisation::GpsLocation&>()))
            .WillRepeatedly(ReleaseSemaphore(&semaphore));

    std::shared_ptr<ISubscriptionListener<types::Localisation::GpsLocation> > subscriptionListener(
                    mockListener);
    // Provider: (runtime1)

    std::shared_ptr<tests::testProvider> testProvider(new tests::DefaulttestProvider());
    //MockGpsProvider* gpsProvider = new MockGpsProvider();
    types::Localisation::GpsLocation gpsLocation1;
    runtime1->registerProvider<tests::testProvider>(domainName, testProvider);

    //This wait is necessary, because registerProvider is async, and a lookup could occur
    // before the register has finished. See Joynr 805 for details
    std::this_thread::sleep_for(std::chrono::seconds(5));

    ProxyBuilder<tests::testProxy>* testProxyBuilder = runtime2->createProxyBuilder<tests::testProxy>(domainName);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeout(1000);

    std::uint64_t qosRoundTripTTL = 40000;

    // Send a message and expect to get a result
    std::shared_ptr<tests::testProxy> gpsProxy(testProxyBuilder
                                               ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                               ->setCached(false)
                                               ->setDiscoveryQos(discoveryQos)
                                               ->build());
    OnChangeWithKeepAliveSubscriptionQos subscriptionQos(
                                    9000,   // validity_ms
                                    1000,    // minInterval_ms
                                    2000,   //  maxInterval_ms
                                    10000);  // alertInterval_ms
    std::string subscriptionId = gpsProxy->subscribeToLocation(subscriptionListener, subscriptionQos);

    // Wait for 2 subscription messages to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(20)));
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(20)));

    gpsProxy->unsubscribeFromLocation(subscriptionId);

    // Check that the unsubscribe is eventually successful
    ASSERT_FALSE(semaphore.waitFor(std::chrono::seconds(10)));
    ASSERT_FALSE(semaphore.waitFor(std::chrono::seconds(10)));
    ASSERT_FALSE(semaphore.waitFor(std::chrono::seconds(10)));
    ASSERT_FALSE(semaphore.waitFor(std::chrono::seconds(10)));

    delete testProxyBuilder;
}

TEST_F(CombinedEnd2EndTest, deleteChannelViaReceiver) {

    // Provider: (runtime1)

    std::shared_ptr<tests::testProvider> testProvider(new tests::DefaulttestProvider());
    //MockGpsProvider* gpsProvider = new MockGpsProvider();
    runtime1->registerProvider<tests::testProvider>(domainName, testProvider);

    std::this_thread::sleep_for(std::chrono::seconds(1)); //This wait is necessary, because registerProvider is async, and a lookup could occour before the register has finished.

    ProxyBuilder<tests::testProxy>* testProxyBuilder = runtime2->createProxyBuilder<tests::testProxy>(domainName);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeout(1000);

    std::uint64_t qosRoundTripTTL = 40000;

    // Send a message and expect to get a result
    std::shared_ptr<tests::testProxy> testProxy(testProxyBuilder
                                               ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                               ->setCached(false)
                                               ->setDiscoveryQos(discoveryQos)
                                               ->build());
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    std::shared_ptr<Future<int> > testFuture(testProxy->addNumbersAsync(1, 2, 3));
    testFuture->wait();

    // runtime1->deleteChannel();
    // runtime2->deleteChannel();

    std::shared_ptr<Future<int> > gpsFuture2(testProxy->addNumbersAsync(1, 2, 3));
    gpsFuture2->wait(1000);

    delete testProxyBuilder;
}

tests::testProxy* createTestProxy(JoynrRuntime *runtime, const std::string& domainName){
    ProxyBuilder<tests::testProxy>* testProxyBuilder
           = runtime->createProxyBuilder<tests::testProxy>(domainName);
   DiscoveryQos discoveryQos;
   discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
   discoveryQos.setDiscoveryTimeout(1000);

   std::uint64_t qosRoundTripTTL = 40000;

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
void subscribeToLocation(std::shared_ptr<ISubscriptionListener<types::Localisation::GpsLocation> > listener,
                            tests::testProxy* testProxy,
                            CombinedEnd2EndTest* testSuite) {
    OnChangeWithKeepAliveSubscriptionQos subscriptionQos(
                                    500000,   // validity_ms
                                    1000,    // minInterval_ms
                                    2000,   //  maxInterval_ms
                                    3000);  // alertInterval_ms
    testSuite->registeredSubscriptionId = testProxy->subscribeToLocation(listener, subscriptionQos);
}

// A function that subscribes to a GpsPosition - to be run in a background thread
static void unsubscribeFromLocation(tests::testProxy* testProxy,
                            std::string subscriptionId) {
    testProxy->unsubscribeFromLocation(subscriptionId);
}


// This test was written to model a bug report where a subscription started in a background thread
// causes a runtime error to be reported by Qt
TEST_F(CombinedEnd2EndTest, subscribeInBackgroundThread) {
    MockGpsSubscriptionListener* mockListener = new MockGpsSubscriptionListener();

    // Use a semaphore to count and wait on calls to the mock listener
    // Semaphore semaphore(0);
    EXPECT_CALL(*mockListener, onReceive(A<const types::Localisation::GpsLocation&>()))
            .WillRepeatedly(ReleaseSemaphore(&semaphore));

    std::shared_ptr<ISubscriptionListener<types::Localisation::GpsLocation> > subscriptionListener(
                    mockListener);

    std::shared_ptr<tests::testProvider> testProvider(new tests::DefaulttestProvider());
    std::string providerParticipantId = runtime1->registerProvider<tests::testProvider>(
            domainName,
            testProvider
    );

    //This wait is necessary, because registerProvider is async, and a lookup could occur
    // before the register has finished.
    std::this_thread::sleep_for(std::chrono::seconds(5));

    tests::testProxy* testProxy = createTestProxy(runtime2, domainName);
    // Subscribe in a background thread
    QtConcurrent::run(subscribeToLocation, subscriptionListener, testProxy, this);

    // Wait for 2 subscription messages to arrive
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(20)));
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(20)));

    unsubscribeFromLocation(testProxy, registeredSubscriptionId);

    runtime1->unregisterProvider(providerParticipantId);
}

TEST_F(CombinedEnd2EndTest, call_async_void_operation) {
    types::ProviderQos providerQos;
    providerQos.setPriority(2);
    std::shared_ptr<tests::testProvider> testProvider(new MockTestProvider(providerQos));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    runtime1->registerProvider<tests::testProvider>(domainName, testProvider);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    ProxyBuilder<tests::testProxy>* testProxyBuilder = runtime2->createProxyBuilder<tests::testProxy>(domainName);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeout(1000);

    std::uint64_t qosRoundTripTTL = 20000;

    // Send a message and expect to get a result
    std::shared_ptr<tests::testProxy> testProxy(testProxyBuilder
                                                   ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                                   ->setCached(false)
                                                   ->setDiscoveryQos(discoveryQos)
                                                   ->build());

    // Setup a callbackFct
    std::function<void()> onSuccess = [] (){
       SUCCEED();
    };
    std::function<void(const exceptions::JoynrException& error)> onError =
            [] (const exceptions::JoynrException& error){
        FAIL();
    };

    // Asynchonously call the void operation
    std::shared_ptr<Future<void> > future (testProxy->voidOperationAsync(onSuccess, onError));

    // Wait for the operation to finish and check for a successful callback
    future->wait();
    ASSERT_TRUE(future->getStatus().successful());

    delete testProxyBuilder;
}

TEST_F(CombinedEnd2EndTest, call_async_void_operation_failure) {
    types::ProviderQos providerQos;
    providerQos.setPriority(2);
    std::shared_ptr<tests::testProvider> testProvider(new MockTestProvider(providerQos));

    std::this_thread::sleep_for(std::chrono::milliseconds(2550));

    std::string testProviderParticipantId = runtime1->registerProvider<tests::testProvider>(
            domainName,
            testProvider
    );

    std::this_thread::sleep_for(std::chrono::milliseconds(2550));

    ProxyBuilder<tests::testProxy>* testProxyBuilder = runtime2->createProxyBuilder<tests::testProxy>(domainName);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeout(1000);

    std::uint64_t qosRoundTripTTL = 20000;

    // Send a message and expect to get a result
    std::shared_ptr<tests::testProxy> testProxy(testProxyBuilder
                                                   ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                                   ->setCached(false)
                                                   ->setDiscoveryQos(discoveryQos)
                                                   ->build());

    // Shut down the provider
    //runtime1->stopMessaging();
    runtime1->unregisterProvider(domainName, testProvider);
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Setup an onError callback function
    std::function<void(const exceptions::JoynrException&)> onError =
            [] (const exceptions::JoynrException& error) {
        EXPECT_EQ(exceptions::JoynrTimeOutException::TYPE_NAME, error.getTypeName());
    };

    // Asynchonously call the void operation
    std::shared_ptr<Future<void> > future (testProxy->voidOperationAsync(nullptr, onError));

    // Wait for the operation to finish and check for a failure callback
    future->wait();
    ASSERT_FALSE(future->getStatus().successful());
    try {
        future->get();
        ADD_FAILURE();
    } catch (exceptions::JoynrTimeOutException& e) {
    }

    runtime1->unregisterProvider(testProviderParticipantId);

    delete testProxyBuilder;
}
