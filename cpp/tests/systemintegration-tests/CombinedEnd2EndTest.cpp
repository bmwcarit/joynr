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
#include <cstdint>
#include <chrono>
#include <memory>
#include <string>
#include <tuple>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "tests/mock/MockSubscriptionListener.h"
#include "joynr/JoynrRuntime.h"
#include "joynr/LibjoynrSettings.h"
#include "joynr/exceptions/MethodInvocationException.h"
#include "joynr/tests/testProxy.h"
#include "joynr/tests/testTypes/DerivedStruct.h"
#include "joynr/tests/testTypes/AnotherDerivedStruct.h"
#include "joynr/types/Localisation/Trip.h"
#include "joynr/types/Localisation/GpsLocation.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/Future.h"
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"
#include "joynr/OnChangeSubscriptionQos.h"

#include "tests/JoynrTest.h"
#include "tests/PrettyPrint.h"
#include "tests/mock/MockTestProvider.h"
#include "tests/utils/PtrUtils.h"

using namespace ::testing;
using namespace joynr;

/*
 * This test creates two Runtimes and will test communication
 * between the two Runtimes via MqttReceiver
 *
 */
class CombinedEnd2EndTest : public testing::TestWithParam<std::tuple<std::string, std::string>>
{
public:
    CombinedEnd2EndTest() : _semaphore(0)
    {
        auto baseUuid = util::createUuid();
        _uuid = "_" + baseUuid.substr(1, baseUuid.length() - 2);
        _domainName = "cppCombinedEnd2EndTest_Domain" + _uuid;
        _discoveryQos.setDiscoveryTimeoutMs(30000);
        _discoveryQos.setRetryIntervalMs(500);
        _discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    }

    ~CombinedEnd2EndTest() override = default;

    void SetUp() override
    {
        const auto messagingSettingsFile1 = std::get<0>(GetParam());
        _runtime1 =
                JoynrRuntime::createRuntime("test-resources/libjoynrSystemIntegration1.settings",
                                            failOnFatalRuntimeError,
                                            messagingSettingsFile1);
        const auto messagingSettingsFile2 = std::get<1>(GetParam());
        _runtime2 =
                JoynrRuntime::createRuntime("test-resources/libjoynrSystemIntegration2.settings",
                                            failOnFatalRuntimeError,
                                            messagingSettingsFile2);
    }

    void TearDown() override
    {
        test::util::resetAndWaitUntilDestroyed(_runtime1);
        test::util::resetAndWaitUntilDestroyed(_runtime2);

        // Delete the persisted participant ids so that each test uses different participant ids
        test::util::removeAllCreatedSettingsAndPersistencyFiles();
    }

protected:
    std::shared_ptr<JoynrRuntime> _runtime1;
    std::shared_ptr<JoynrRuntime> _runtime2;
    std::string _uuid;
    std::string _domainName;
    Semaphore _semaphore;
    DiscoveryQos _discoveryQos;
};

TEST_P(CombinedEnd2EndTest, surviveDestructionOfRuntime)
{
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // consumer for testinterface
    {
        std::shared_ptr<ProxyBuilder<tests::testProxy>> testProxyBuilder =
                _runtime2->createProxyBuilder<tests::testProxy>(_domainName);
        std::uint64_t qosRoundTripTTL = 40000;

        // destroy runtime
        test::util::resetAndWaitUntilDestroyed(_runtime2);

        // try to build a proxy, it must not run into SIGSEGV
        EXPECT_THROW(std::shared_ptr<tests::testProxy> testProxy(
                             testProxyBuilder->setMessagingQos(MessagingQos(qosRoundTripTTL))
                                     ->setDiscoveryQos(_discoveryQos)
                                     ->build()),
                     exceptions::DiscoveryException);
    }
}

TEST_P(CombinedEnd2EndTest, callRpcMethodViaMqttReceiverAndReceiveReply)
{

    // Provider: (_runtime1)
    types::ProviderQos providerQos;
    providerQos.setPriority(2);
    auto testProvider = std::make_shared<MockTestProvider>();

    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::string participantId = _runtime1->registerProvider<tests::testProvider>(
            _domainName, testProvider, providerQos);

    // consumer for testinterface
    // Testing Lists
    {
        std::shared_ptr<ProxyBuilder<tests::testProxy>> testProxyBuilder =
                _runtime2->createProxyBuilder<tests::testProxy>(_domainName);
        std::uint64_t qosRoundTripTTL = 40000;

        // Send a message and expect to get a result
        std::shared_ptr<tests::testProxy> testProxy(
                testProxyBuilder->setMessagingQos(MessagingQos(qosRoundTripTTL))
                        ->setDiscoveryQos(_discoveryQos)
                        ->build());

        std::vector<int> list;
        list.push_back(2);
        list.push_back(4);
        list.push_back(8);
        std::shared_ptr<Future<int>> gpsFuture(testProxy->sumIntsAsync(list));
        gpsFuture->wait();
        int expectedValue = 2 + 4 + 8;
        ASSERT_EQ(StatusCodeEnum::SUCCESS, gpsFuture->getStatus());
        int actualValue;
        gpsFuture->get(actualValue);
        EXPECT_EQ(expectedValue, actualValue);
        // TODO CA: shared pointer for proxy builder?

        /*
         * Testing TRIP
         * Now try to send a Trip (which contains a list) and check if the returned trip is
         * identical.
         */

        std::vector<types::Localisation::GpsLocation> inputLocationList;
        inputLocationList.push_back(
                types::Localisation::GpsLocation(1.1,
                                                 2.2,
                                                 3.3,
                                                 types::Localisation::GpsFixEnum::MODE2D,
                                                 0.0,
                                                 0.0,
                                                 0.0,
                                                 0.0,
                                                 444,
                                                 444,
                                                 4));
        inputLocationList.push_back(
                types::Localisation::GpsLocation(1.1,
                                                 2.2,
                                                 3.3,
                                                 types::Localisation::GpsFixEnum::MODE2D,
                                                 0.0,
                                                 0.0,
                                                 0.0,
                                                 0.0,
                                                 444,
                                                 444,
                                                 5));
        inputLocationList.push_back(
                types::Localisation::GpsLocation(1.1,
                                                 2.2,
                                                 3.3,
                                                 types::Localisation::GpsFixEnum::MODE2D,
                                                 0.0,
                                                 0.0,
                                                 0.0,
                                                 0.0,
                                                 444,
                                                 444,
                                                 6));
        types::Localisation::Trip inputTrip;
        inputTrip.setLocations(inputLocationList);
        std::shared_ptr<Future<types::Localisation::Trip>> tripFuture(
                testProxy->optimizeTripAsync(inputTrip));
        tripFuture->wait();
        ASSERT_EQ(StatusCodeEnum::SUCCESS, tripFuture->getStatus());
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
        } catch (const exceptions::JoynrException& e) {
            FAIL() << "returnPrimeNumbers was not successful";
        }

        /*
         * Testing List of Locations
         * Now try to send a List of GpsLocations and see if it is returned correctly.
         */
        std::vector<types::Localisation::GpsLocation> inputGpsLocationList;
        inputGpsLocationList.push_back(
                types::Localisation::GpsLocation(1.1,
                                                 2.2,
                                                 3.3,
                                                 types::Localisation::GpsFixEnum::MODE2D,
                                                 0.0,
                                                 0.0,
                                                 0.0,
                                                 0.0,
                                                 444,
                                                 444,
                                                 4));
        inputGpsLocationList.push_back(
                types::Localisation::GpsLocation(1.1,
                                                 2.2,
                                                 3.3,
                                                 types::Localisation::GpsFixEnum::MODE2D,
                                                 0.0,
                                                 0.0,
                                                 0.0,
                                                 0.0,
                                                 444,
                                                 444,
                                                 5));
        inputGpsLocationList.push_back(
                types::Localisation::GpsLocation(1.1,
                                                 2.2,
                                                 3.3,
                                                 types::Localisation::GpsFixEnum::MODE2D,
                                                 0.0,
                                                 0.0,
                                                 0.0,
                                                 0.0,
                                                 444,
                                                 444,
                                                 6));
        std::shared_ptr<Future<std::vector<types::Localisation::GpsLocation>>> listLocationFuture(
                testProxy->optimizeLocationListAsync(inputGpsLocationList));
        listLocationFuture->wait();
        ASSERT_EQ(StatusCodeEnum::SUCCESS, tripFuture->getStatus());
        std::vector<types::Localisation::GpsLocation> actualLocation;
        listLocationFuture->get(actualLocation);
        EXPECT_EQ(inputGpsLocationList, actualLocation);

        /*
         * Testing GetAttribute, when setAttribute has been called locally.
         *
         */
        // Primes
        int testPrimeValue = 15;

        std::function<void()> onSuccess = []() {};

        std::function<void(const exceptions::ProviderRuntimeException&)> onError =
                [](const exceptions::ProviderRuntimeException&) {};

        /*
         * because of the implementation of the MockTestProvider,
         * we can use the async API of the testProvider in a sync way
         */
        testProvider->setFirstPrime(testPrimeValue, onSuccess, onError);

        int primeResult(0);
        JOYNR_ASSERT_NO_THROW({ testProxy->getFirstPrime(primeResult); });
        EXPECT_EQ(primeResult, 15);

        // List of strings,
        std::vector<std::string> localStrList;
        std::vector<std::string> remoteStrList;
        localStrList.push_back("one ü");
        localStrList.push_back("two 漢語");
        localStrList.push_back("three ـتـ");
        localStrList.push_back("four {");
        testProvider->setListOfStrings(localStrList, onSuccess, onError);

        JOYNR_ASSERT_NO_THROW({ testProxy->getListOfStrings(remoteStrList); });
        EXPECT_EQ(localStrList, remoteStrList);

        /*
         * Testing GetAttribute with Remote SetAttribute
         *
         */
        JOYNR_ASSERT_NO_THROW({ testProxy->setFirstPrime(19); });
        JOYNR_ASSERT_NO_THROW({ testProxy->getFirstPrime(primeResult); });
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
        } catch (const exceptions::JoynrException& e) {
            FAIL() << "getListOfInts was not successful";
        }
        EXPECT_EQ(outputIntLIst, inputIntList);
        EXPECT_EQ(outputIntLIst.at(1), 3);
        // test remote setter
        inputIntList.clear();
        inputIntList.push_back(7);
        inputIntList.push_back(11);
        inputIntList.push_back(13);
        JOYNR_ASSERT_NO_THROW({ testProxy->setListOfInts(inputIntList); });
        JOYNR_ASSERT_NO_THROW({ testProxy->getListOfInts(outputIntLIst); });
        EXPECT_EQ(outputIntLIst, inputIntList);
        EXPECT_EQ(outputIntLIst.at(1), 11);

        // Testing enums
        testProxy->setEnumAttribute(tests::testTypes::TestEnum::TWO);
        tests::testTypes::TestEnum::Enum actualEnumAttribute;
        testProxy->getEnumAttribute(actualEnumAttribute);
        EXPECT_EQ(actualEnumAttribute, tests::testTypes::TestEnum::TWO);
        EXPECT_THROW(
                testProxy->setEnumAttribute(static_cast<tests::testTypes::TestEnum::Enum>(999)),
                exceptions::MethodInvocationException);

        // Testing byte buffer
        ByteBuffer byteBufferValue{1, 2, 3};
        testProxy->setByteBufferAttribute(byteBufferValue);
        ByteBuffer actualByteBufferValue;
        testProxy->getByteBufferAttribute(actualByteBufferValue);
        EXPECT_EQ(actualByteBufferValue, byteBufferValue);

        ByteBuffer returnByteBufferValue;
        testProxy->methodWithByteBuffer(returnByteBufferValue, byteBufferValue);
        EXPECT_EQ(returnByteBufferValue, byteBufferValue);
    }

    // Testing TTL
    {
        // create a proxy with very short TTL and expect no returning replies.
        std::shared_ptr<ProxyBuilder<tests::testProxy>> testProxyBuilder =
                _runtime2->createProxyBuilder<tests::testProxy>(_domainName);

        std::uint64_t qosRoundTripTTL = 1;
        std::shared_ptr<tests::testProxy> testProxy(
                testProxyBuilder->setMessagingQos(MessagingQos(qosRoundTripTTL))
                        ->setDiscoveryQos(_discoveryQos)
                        ->build());
        std::shared_ptr<Future<int>> testFuture(testProxy->addNumbersAsync(1, 2, 3));
        testFuture->wait();
        ASSERT_EQ(StatusCodeEnum::ERROR, testFuture->getStatus());
    }

    // TESTING Attribute getter of an array of a nested struct
    {
        std::shared_ptr<ProxyBuilder<tests::testProxy>> testProxyBuilder =
                _runtime2->createProxyBuilder<tests::testProxy>(_domainName);

        std::uint64_t qosRoundTripTTL = 40000;

        // Send a message and expect to get a result
        std::shared_ptr<tests::testProxy> testProxy(
                testProxyBuilder->setMessagingQos(MessagingQos(qosRoundTripTTL))
                        ->setDiscoveryQos(_discoveryQos)
                        ->build());
        std::vector<tests::testTypes::HavingComplexArrayMemberStruct> setValue;
        std::vector<tests::testTypes::NeverUsedAsAttributeTypeOrMethodParameterStruct> arrayMember;
        arrayMember.push_back(
                tests::testTypes::NeverUsedAsAttributeTypeOrMethodParameterStruct("neverUsed"));
        setValue.push_back(tests::testTypes::HavingComplexArrayMemberStruct(arrayMember));
        testProxy->setAttributeArrayOfNestedStructs(setValue);

        std::vector<tests::testTypes::HavingComplexArrayMemberStruct> result;
        testProxy->getAttributeArrayOfNestedStructs(result);
        ASSERT_EQ(result, setValue);
    }

    // TESTING getter/setter and operation calls with different kinds of parameters (maps, complex
    // structs, ...)
    {
        using namespace types::TestTypes;
        std::shared_ptr<ProxyBuilder<tests::testProxy>> testProxyBuilder =
                _runtime2->createProxyBuilder<tests::testProxy>(_domainName);

        std::uint64_t qosRoundTripTTL = 40000;

        // Send a message and expect to get a result
        std::shared_ptr<tests::testProxy> testProxy =
                testProxyBuilder->setMessagingQos(MessagingQos(qosRoundTripTTL))
                        ->setDiscoveryQos(_discoveryQos)
                        ->build();

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

    // Testing operation overloading
    {
        std::shared_ptr<ProxyBuilder<tests::testProxy>> testProxyBuilder =
                _runtime2->createProxyBuilder<tests::testProxy>(_domainName);

        std::uint64_t qosRoundTripTTL = 40000;

        // Send a message and expect to get a result
        std::shared_ptr<tests::testProxy> testProxy =
                testProxyBuilder->setMessagingQos(MessagingQos(qosRoundTripTTL))
                        ->setDiscoveryQos(_discoveryQos)
                        ->build();

        std::string derivedStructResult;
        std::string anotherDerivedStructResult;

        // Check that the operation overloading worked and the result is of the correct type
        testProxy->overloadedOperation(derivedStructResult, tests::testTypes::DerivedStruct());
        testProxy->overloadedOperation(
                anotherDerivedStructResult, tests::testTypes::AnotherDerivedStruct());
        EXPECT_EQ(derivedStructResult, "DerivedStruct");
        EXPECT_EQ(anotherDerivedStructResult, "AnotherDerivedStruct");
    }
    _runtime1->unregisterProvider(participantId);
}

TEST_P(CombinedEnd2EndTest, subscribeViaMqttReceiverAndReceiveReply)
{

    auto subscriptionListener = std::make_shared<MockGpsSubscriptionListener>();

    // Use a _semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*subscriptionListener, onReceive(A<const types::Localisation::GpsLocation&>()))
            .WillRepeatedly(ReleaseSemaphore(&_semaphore));

    // Provider: (_runtime1)

    auto testProvider = std::make_shared<tests::DefaulttestProvider>();
    types::ProviderQos providerQos;
    std::chrono::milliseconds millisSinceEpoch =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch());
    providerQos.setPriority(millisSinceEpoch.count());
    providerQos.setScope(types::ProviderScope::GLOBAL);
    providerQos.setSupportsOnChangeSubscriptions(true);
    std::string participantId = _runtime1->registerProvider<tests::testProvider>(
            _domainName, testProvider, providerQos);

    std::shared_ptr<ProxyBuilder<tests::testProxy>> testProxyBuilder =
            _runtime2->createProxyBuilder<tests::testProxy>(_domainName);

    std::uint64_t qosRoundTripTTL = 40000;

    // Send a message and expect to get a result
    std::shared_ptr<tests::testProxy> testProxy(
            testProxyBuilder->setMessagingQos(MessagingQos(qosRoundTripTTL))
                    ->setDiscoveryQos(_discoveryQos)
                    ->build());

    std::int64_t minInterval_ms = 1000;
    std::int64_t maxInterval_ms = 2000;

    auto subscriptionQos =
            std::make_shared<OnChangeWithKeepAliveSubscriptionQos>(10000, // validity_ms
                                                                   1000,  // publication ttl
                                                                   minInterval_ms,
                                                                   maxInterval_ms,
                                                                   3000); // alertInterval_ms
    std::shared_ptr<Future<std::string>> future =
            testProxy->subscribeToLocation(subscriptionListener, subscriptionQos);

    std::string subscriptionId;
    JOYNR_ASSERT_NO_THROW({ future->get(5000, subscriptionId); });
    // Wait for 2 subscription messages to arrive
    ASSERT_TRUE(_semaphore.waitFor(std::chrono::seconds(20)));
    ASSERT_TRUE(_semaphore.waitFor(std::chrono::seconds(20)));

    testProxy->unsubscribeFromLocation(subscriptionId);
    _runtime1->unregisterProvider(participantId);
}

TEST_P(CombinedEnd2EndTest, callFireAndForgetMethod)
{
    std::int32_t expectedIntParam = 42;
    std::string expectedStringParam = "CombinedEnd2EndTest::callFireAndForgetMethod";
    tests::testTypes::ComplexTestType expectedComplexParam;

    // Provider: (_runtime1)
    auto testProvider = std::make_shared<MockTestProvider>();
    EXPECT_CALL(*testProvider,
                methodFireAndForget(expectedIntParam, expectedStringParam, expectedComplexParam))
            .WillOnce(ReleaseSemaphore(&_semaphore));
    types::ProviderQos providerQos;
    std::chrono::milliseconds millisSinceEpoch =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch());
    providerQos.setPriority(millisSinceEpoch.count());
    providerQos.setScope(types::ProviderScope::GLOBAL);
    providerQos.setSupportsOnChangeSubscriptions(true);
    std::string participantId = _runtime1->registerProvider<tests::testProvider>(
            _domainName, testProvider, providerQos);

    std::shared_ptr<ProxyBuilder<tests::testProxy>> testProxyBuilder =
            _runtime2->createProxyBuilder<tests::testProxy>(_domainName);

    std::uint64_t qosRoundTripTTL = 40000;

    // Send a message and expect to get a result
    std::shared_ptr<tests::testProxy> testProxy(
            testProxyBuilder->setMessagingQos(MessagingQos(qosRoundTripTTL))
                    ->setDiscoveryQos(_discoveryQos)
                    ->build());

    testProxy->methodFireAndForget(expectedIntParam, expectedStringParam, expectedComplexParam);

    ASSERT_TRUE(_semaphore.waitFor(std::chrono::seconds(20)));
    _runtime1->unregisterProvider(participantId);
}

TEST_P(CombinedEnd2EndTest, subscribeToOnChange)
{
    auto subscriptionListener = std::make_shared<MockGpsSubscriptionListener>();

    // Use a _semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*subscriptionListener, onReceive(A<const types::Localisation::GpsLocation&>()))
            .Times(4)
            .WillRepeatedly(ReleaseSemaphore(&_semaphore));

    // Provider: (_runtime1)

    auto testProvider = std::make_shared<tests::DefaulttestProvider>();
    types::ProviderQos providerQos;
    std::chrono::milliseconds millisSinceEpoch =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch());
    providerQos.setPriority(millisSinceEpoch.count());
    providerQos.setScope(types::ProviderScope::GLOBAL);
    providerQos.setSupportsOnChangeSubscriptions(true);

    const bool persist = true;
    const bool awaitGlobalRegistration = true;
    std::string participantId = _runtime1->registerProvider<tests::testProvider>(
            _domainName, testProvider, providerQos, persist, awaitGlobalRegistration);

    std::shared_ptr<ProxyBuilder<tests::testProxy>> testProxyBuilder =
            _runtime2->createProxyBuilder<tests::testProxy>(_domainName);

    std::uint64_t qosRoundTripTTL = 40000;

    // Send a message and expect to get a result
    std::shared_ptr<tests::testProxy> testProxy(
            testProxyBuilder->setMessagingQos(MessagingQos(qosRoundTripTTL))
                    ->setDiscoveryQos(_discoveryQos)
                    ->build());

    // Publications will be sent maintaining this minimum interval provided, even if the value
    // changes more often. This prevents the consumer from being flooded by updated values.
    // The filtering happens on the provider's side, thus also preventing excessive network traffic.
    // This value is provided in milliseconds. The minimum value for minInterval is 0 ms.
    // Make sure this test fails when the min value is changed in onChangeSubscriptionQos
    ASSERT_LE(OnChangeSubscriptionQos::MIN_MIN_INTERVAL_MS(), 0);

    std::int64_t minInterval_ms = 0;
    auto subscriptionQos =
            std::make_shared<OnChangeSubscriptionQos>(500000,          // validity_ms
                                                      10000,           // publication ttl
                                                      minInterval_ms); // minInterval_ms
    auto future = testProxy->subscribeToLocation(subscriptionListener, subscriptionQos);

    std::string subscriptionId;
    JOYNR_ASSERT_NO_THROW({ future->get(10000, subscriptionId); });

    auto invokeSetter = [testProvider](std::int64_t value) {
        auto onSuccess = []() {};
        types::Localisation::GpsLocation location;
        location.setDeviceTime(value);
        testProvider->setLocation(location, onSuccess, nullptr);
    };

    // Wait for initial publication to arrive
    ASSERT_TRUE(_semaphore.waitFor(std::chrono::seconds(20)));

    // Change the location multiple times
    invokeSetter(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms + 100));
    invokeSetter(2);
    std::this_thread::sleep_for(std::chrono::milliseconds(minInterval_ms + 100));
    invokeSetter(3);

    // Wait for 3 subscription messages to arrive
    ASSERT_TRUE(_semaphore.waitFor(std::chrono::seconds(20)));
    ASSERT_TRUE(_semaphore.waitFor(std::chrono::seconds(20)));
    ASSERT_TRUE(_semaphore.waitFor(std::chrono::seconds(20)));
    JOYNR_ASSERT_NO_THROW({ testProxy->unsubscribeFromLocation(subscriptionId); });
    _runtime1->unregisterProvider(participantId);
}

TEST_P(CombinedEnd2EndTest, subscribeToListAttribute)
{

    auto subscriptionListener =
            std::make_shared<MockSubscriptionListenerOneType<std::vector<int>>>();

    std::vector<int> expectedValues = {1000, 2000, 3000};
    // Use a _semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*subscriptionListener, onReceive(Eq(expectedValues)))
            .WillRepeatedly(ReleaseSemaphore(&_semaphore));

    // Provider: (_runtime1)

    types::ProviderQos providerQos;
    providerQos.setPriority(2);
    auto testProvider = std::make_shared<MockTestProvider>();
    testProvider->setListOfInts(
            expectedValues, []() {}, [](const exceptions::JoynrRuntimeException&) {});
    std::string providerParticipantId = _runtime1->registerProvider<tests::testProvider>(
            _domainName, testProvider, providerQos);

    std::shared_ptr<ProxyBuilder<tests::testProxy>> proxyBuilder =
            _runtime2->createProxyBuilder<tests::testProxy>(_domainName);

    std::uint64_t qosRoundTripTTL = 40000;

    // Send a message and expect to get a result
    std::shared_ptr<tests::testProxy> testProxy(
            proxyBuilder->setMessagingQos(MessagingQos(qosRoundTripTTL))
                    ->setDiscoveryQos(_discoveryQos)
                    ->build());

    auto subscriptionQos =
            std::make_shared<OnChangeWithKeepAliveSubscriptionQos>(500000, // validity_ms
                                                                   1000,   // publication ttl
                                                                   1000,   // minInterval_ms
                                                                   2000,   // maxInterval_ms
                                                                   3000);  // alertInterval_ms
    auto future = testProxy->subscribeToListOfInts(subscriptionListener, subscriptionQos);

    std::string subscriptionId;
    JOYNR_ASSERT_NO_THROW({ future->get(5000, subscriptionId); });

    // Wait for 2 subscription messages to arrive
    ASSERT_TRUE(_semaphore.waitFor(std::chrono::seconds(20)));
    ASSERT_TRUE(_semaphore.waitFor(std::chrono::seconds(20)));

    testProxy->unsubscribeFromListOfInts(subscriptionId);
    _runtime1->unregisterProvider(providerParticipantId);
}

TEST_P(CombinedEnd2EndTest, buildProxyForNonExistentDomain_throwsDiscoveryException)
{
    // Create a proxy to a non-existent domain
    const std::string nonexistentDomain(std::string("non-existent-").append(_uuid));
    std::shared_ptr<ProxyBuilder<tests::testProxy>> testProxyBuilder =
            _runtime2->createProxyBuilder<tests::testProxy>(nonexistentDomain);
    const std::uint64_t qosRoundTripTTL = 40000;
    const int discoveryTimeoutMs = 5000;
    const int retryIntervalMs = discoveryTimeoutMs + 1; // no retry
    DiscoveryQos discoveryQosOtherTimeout;
    discoveryQosOtherTimeout.setArbitrationStrategy(
            DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQosOtherTimeout.setDiscoveryTimeoutMs(discoveryTimeoutMs);
    discoveryQosOtherTimeout.setRetryIntervalMs(retryIntervalMs);

    // Time how long arbitration takes
    auto start = std::chrono::system_clock::now();
    int elapsed = 0;

    // Expect a DiscoveryException
    try {
        std::shared_ptr<tests::testProxy> testProxy(
                testProxyBuilder->setMessagingQos(MessagingQos(qosRoundTripTTL))
                        ->setDiscoveryQos(discoveryQosOtherTimeout)
                        ->build());
        std::ignore = testProxy;
        FAIL() << "unexpected success, expected DiscoveryException";
    } catch (const exceptions::DiscoveryException& e) {
        auto now = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
        elapsed = duration.count();
        ASSERT_LT(elapsed, discoveryTimeoutMs) << "Expected joynr::exceptions::DiscoveryException "
                                                  "has been thrown too late. Message: " +
                                                          e.getMessage();
    }
}

TEST_P(CombinedEnd2EndTest, registerAndLookupWithGbids_callMethod)
{
    const std::vector<std::string> gbids{"joynrdefaultgbid"};
    // Provider: _runtime1
    types::ProviderQos providerQos;
    auto testProvider = std::make_shared<MockTestProvider>();
    std::string providerParticipantId = _runtime1->registerProvider<tests::testProvider>(
            _domainName, testProvider, providerQos, true, true, gbids);

    // Proxy: _runtime2
    std::shared_ptr<ProxyBuilder<tests::testProxy>> proxyBuilder =
            _runtime2->createProxyBuilder<tests::testProxy>(_domainName);
    std::uint64_t qosRoundTripTTL = 40000;
    std::shared_ptr<tests::testProxy> testProxy(
            proxyBuilder->setMessagingQos(MessagingQos(qosRoundTripTTL))->setGbids(gbids)->build());

    // Send a message and expect to get a result
    std::int32_t result;
    const std::vector<std::int32_t> numbers{1, 2};
    testProxy->sumInts(result, numbers);
    EXPECT_EQ(3, result);

    _runtime1->unregisterProvider(providerParticipantId);
}

TEST_P(CombinedEnd2EndTest, unsubscribeViaMqttReceiver)
{

    MockGpsSubscriptionListener* mockListener = new MockGpsSubscriptionListener();

    // Use a _semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener, onReceive(A<const types::Localisation::GpsLocation&>()))
            .WillRepeatedly(ReleaseSemaphore(&_semaphore));

    std::shared_ptr<ISubscriptionListener<types::Localisation::GpsLocation>> subscriptionListener(
            mockListener);
    // Provider: (_runtime1)

    auto testProvider = std::make_shared<tests::DefaulttestProvider>();
    // MockGpsProvider* gpsProvider = new MockGpsProvider();
    types::Localisation::GpsLocation gpsLocation1;
    types::ProviderQos providerQos;
    std::chrono::milliseconds millisSinceEpoch =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch());
    providerQos.setPriority(millisSinceEpoch.count());
    providerQos.setScope(types::ProviderScope::GLOBAL);
    providerQos.setSupportsOnChangeSubscriptions(true);
    std::string participantId = _runtime1->registerProvider<tests::testProvider>(
            _domainName, testProvider, providerQos);

    std::shared_ptr<ProxyBuilder<tests::testProxy>> testProxyBuilder =
            _runtime2->createProxyBuilder<tests::testProxy>(_domainName);

    std::uint64_t qosRoundTripTTL = 40000;

    // Send a message and expect to get a result
    std::shared_ptr<tests::testProxy> gpsProxy(
            testProxyBuilder->setMessagingQos(MessagingQos(qosRoundTripTTL))
                    ->setDiscoveryQos(_discoveryQos)
                    ->build());
    auto subscriptionQos =
            std::make_shared<OnChangeWithKeepAliveSubscriptionQos>(9000,   // validity_ms
                                                                   1000,   // publication ttl
                                                                   1000,   // minInterval_ms
                                                                   2000,   //  maxInterval_ms
                                                                   10000); // alertInterval_ms
    auto future = gpsProxy->subscribeToLocation(subscriptionListener, subscriptionQos);

    std::string subscriptionId;
    JOYNR_ASSERT_NO_THROW({ future->get(5000, subscriptionId); });

    // Wait for 2 subscription messages to arrive
    ASSERT_TRUE(_semaphore.waitFor(std::chrono::seconds(20)));
    ASSERT_TRUE(_semaphore.waitFor(std::chrono::seconds(20)));

    gpsProxy->unsubscribeFromLocation(subscriptionId);

    // Check that the unsubscribe is eventually successful
    ASSERT_FALSE(_semaphore.waitFor(std::chrono::seconds(10)));
    ASSERT_FALSE(_semaphore.waitFor(std::chrono::seconds(10)));
    ASSERT_FALSE(_semaphore.waitFor(std::chrono::seconds(10)));
    ASSERT_FALSE(_semaphore.waitFor(std::chrono::seconds(10)));
    JOYNR_ASSERT_NO_THROW({ gpsProxy->unsubscribeFromLocation(subscriptionId); });
    _runtime1->unregisterProvider(participantId);
}

TEST_P(CombinedEnd2EndTest, deleteChannelViaReceiver)
{

    // Provider: (_runtime1)

    auto testProvider = std::make_shared<tests::DefaulttestProvider>();
    // MockGpsProvider* gpsProvider = new MockGpsProvider();
    types::ProviderQos providerQos;
    std::chrono::milliseconds millisSinceEpoch =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch());
    providerQos.setPriority(millisSinceEpoch.count());
    providerQos.setScope(types::ProviderScope::GLOBAL);
    providerQos.setSupportsOnChangeSubscriptions(true);
    std::string participantId = _runtime1->registerProvider<tests::testProvider>(
            _domainName, testProvider, providerQos);

    std::shared_ptr<ProxyBuilder<tests::testProxy>> testProxyBuilder =
            _runtime2->createProxyBuilder<tests::testProxy>(_domainName);

    std::uint64_t qosRoundTripTTL = 40000;

    // Send a message and expect to get a result
    std::shared_ptr<tests::testProxy> testProxy(
            testProxyBuilder->setMessagingQos(MessagingQos(qosRoundTripTTL))
                    ->setDiscoveryQos(_discoveryQos)
                    ->build());
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    std::shared_ptr<Future<int>> testFuture(testProxy->addNumbersAsync(1, 2, 3));
    testFuture->wait();

    std::shared_ptr<Future<int>> gpsFuture2(testProxy->addNumbersAsync(1, 2, 3));
    gpsFuture2->wait(1000);
    _runtime1->unregisterProvider(participantId);
}

std::shared_ptr<tests::testProxy> createTestProxy(JoynrRuntime& runtime,
                                                  const std::string& _domainName,
                                                  const DiscoveryQos& _discoveryQos)
{
    std::shared_ptr<ProxyBuilder<tests::testProxy>> testProxyBuilder =
            runtime.createProxyBuilder<tests::testProxy>(_domainName);

    std::uint64_t qosRoundTripTTL = 40000;

    // Send a message and expect to get a result
    std::shared_ptr<tests::testProxy> testProxy(
            testProxyBuilder->setMessagingQos(MessagingQos(qosRoundTripTTL))
                    ->setDiscoveryQos(_discoveryQos)
                    ->build());
    return testProxy;
}

// Needs to be static since not bound to test case runtime (not mandatory to pass the test)
static std::string subscribeInBackgroundThreadId;

// A function that subscribes to a GpsPosition - to be run in a background thread
void subscribeToLocation(
        std::shared_ptr<ISubscriptionListener<types::Localisation::GpsLocation>> listener,
        std::shared_ptr<tests::testProxy> testProxy)
{
    auto subscriptionQos =
            std::make_shared<OnChangeWithKeepAliveSubscriptionQos>(500000, // validity_ms
                                                                   1000,   // publication ttl
                                                                   1000,   // minInterval_ms
                                                                   2000,   //  maxInterval_ms
                                                                   3000);  // alertInterval_ms
    auto future = testProxy->subscribeToLocation(listener, subscriptionQos);
    JOYNR_ASSERT_NO_THROW({ future->get(5000, subscribeInBackgroundThreadId); });
}

// A function that subscribes to a GpsPosition - to be run in a background thread
static void unsubscribeFromLocation(std::shared_ptr<tests::testProxy> testProxy)
{
    testProxy->unsubscribeFromLocation(subscribeInBackgroundThreadId);
}

TEST_P(CombinedEnd2EndTest, subscribeInBackgroundThread)
{
    MockGpsSubscriptionListener* mockListener = new MockGpsSubscriptionListener();

    // Use a _semaphore to count and wait on calls to the mock listener
    // Semaphore _semaphore(0);
    EXPECT_CALL(*mockListener, onReceive(A<const types::Localisation::GpsLocation&>()))
            .WillRepeatedly(ReleaseSemaphore(&_semaphore));

    std::shared_ptr<ISubscriptionListener<types::Localisation::GpsLocation>> subscriptionListener(
            mockListener);

    auto testProvider = std::make_shared<tests::DefaulttestProvider>();
    types::ProviderQos providerQos;
    std::chrono::milliseconds millisSinceEpoch =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch());
    providerQos.setPriority(millisSinceEpoch.count());
    providerQos.setScope(types::ProviderScope::GLOBAL);
    providerQos.setSupportsOnChangeSubscriptions(true);
    std::string providerParticipantId = _runtime1->registerProvider<tests::testProvider>(
            _domainName, testProvider, providerQos);

    std::shared_ptr<tests::testProxy> testProxy =
            createTestProxy(*_runtime2, _domainName, _discoveryQos);
    // Subscribe in a background thread
    // subscribeToLocation(subscriptionListener, testProxy, this);
    std::async(std::launch::async, subscribeToLocation, subscriptionListener, testProxy);

    // Wait for 2 subscription messages to arrive
    ASSERT_TRUE(_semaphore.waitFor(std::chrono::seconds(20)));
    ASSERT_TRUE(_semaphore.waitFor(std::chrono::seconds(20)));

    unsubscribeFromLocation(testProxy);

    _runtime1->unregisterProvider(providerParticipantId);
}

TEST_P(CombinedEnd2EndTest, call_async_void_operation)
{
    types::ProviderQos providerQos;
    providerQos.setPriority(2);
    auto testProvider = std::make_shared<MockTestProvider>();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::string participantId = _runtime1->registerProvider<tests::testProvider>(
            _domainName, testProvider, providerQos);

    std::shared_ptr<ProxyBuilder<tests::testProxy>> testProxyBuilder =
            _runtime2->createProxyBuilder<tests::testProxy>(_domainName);

    std::uint64_t qosRoundTripTTL = 20000;

    // Send a message and expect to get a result
    std::shared_ptr<tests::testProxy> testProxy(
            testProxyBuilder->setMessagingQos(MessagingQos(qosRoundTripTTL))
                    ->setDiscoveryQos(_discoveryQos)
                    ->build());

    // Setup a callbackFct
    std::function<void()> onSuccess = []() { SUCCEED(); };
    std::function<void(const exceptions::JoynrException& error)> onError =
            [](const exceptions::JoynrException&) { FAIL(); };

    // Asynchonously call the void operation
    std::shared_ptr<Future<void>> future(testProxy->voidOperationAsync(onSuccess, onError));

    // Wait for the operation to finish and check for a successful callback
    future->wait();
    ASSERT_EQ(StatusCodeEnum::SUCCESS, future->getStatus());
    _runtime1->unregisterProvider(participantId);
}

TEST_P(CombinedEnd2EndTest, call_async_void_operation_failure)
{
    types::ProviderQos providerQos;
    providerQos.setPriority(2);
    auto testProvider = std::make_shared<MockTestProvider>();

    std::string testProviderParticipantId =
            _runtime1->registerProvider<tests::testProvider>(_domainName,
                                                             testProvider,
                                                             providerQos,
                                                             true, // persist
                                                             true); // awaitGlobalRegistration

    std::shared_ptr<ProxyBuilder<tests::testProxy>> testProxyBuilder =
            _runtime2->createProxyBuilder<tests::testProxy>(_domainName);

    std::uint64_t qosRoundTripTTL = 5000;

    std::shared_ptr<tests::testProxy> testProxy(
            testProxyBuilder->setMessagingQos(MessagingQos(qosRoundTripTTL))
                    ->setDiscoveryQos(_discoveryQos)
                    ->build());

    // Shut down the provider
    // _runtime1->stopExternalCommunication();
    _runtime1->unregisterProvider(_domainName, testProvider);
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Setup an onError callback function
    std::function<void(const exceptions::JoynrException&)> onError =
            [](const exceptions::JoynrException& error) {
        EXPECT_EQ(exceptions::JoynrTimeOutException::TYPE_NAME(), error.getTypeName());
    };

    // Asynchonously call the void operation
    std::shared_ptr<Future<void>> future(testProxy->voidOperationAsync(nullptr, onError));

    // Wait for the operation to finish and check for a failure callback
    future->wait();
    ASSERT_EQ(StatusCodeEnum::ERROR, future->getStatus());
    try {
        future->get();
        ADD_FAILURE();
    } catch (const exceptions::JoynrTimeOutException& e) {
    }
}

using namespace std::string_literals;

INSTANTIATE_TEST_SUITE_P(
        Mqtt,
        CombinedEnd2EndTest,
        testing::Values(std::make_tuple("test-resources/MqttSystemIntegrationTest1.settings"s,
                                        "test-resources/MqttSystemIntegrationTest2.settings"s)));

INSTANTIATE_TEST_SUITE_P(MqttOverTLS,
                        CombinedEnd2EndTest,
                        testing::Values(std::make_tuple(
                                "test-resources/MqttOverTLSSystemIntegrationTest1.settings"s,
                                "test-resources/MqttOverTLSSystemIntegrationTest2.settings"s)));
