/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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

#include "IltAbstractConsumerTest.h"
#include "joynr/ISubscriptionListener.h"
#include "joynr/MulticastSubscriptionQos.h"
#include "joynr/Semaphore.h"
#include "joynr/SubscriptionListener.h"

using namespace ::testing;

class IltConsumerBroadcastSubscriptionTest
        : public IltAbstractConsumerTest<::testing::TestWithParam<std::vector<std::string>>>
{
public:
    IltConsumerBroadcastSubscriptionTest()
            : validity(60000),
              subscriptionQos(std::make_shared<joynr::MulticastSubscriptionQos>(validity))
    {
    }

protected:
    int64_t validity;
    std::shared_ptr<joynr::MulticastSubscriptionQos> subscriptionQos;
};

joynr::Logger iltConsumerBroadcastSubscriptionTestLogger("IltConsumerBroadcastSubscriptionTest");

class MockBroadcastWithSinglePrimitiveParameterBroadcastListener
        : public joynr::ISubscriptionListener<std::string>
{
public:
    MOCK_METHOD1(onSubscribed, void(const std::string& subscriptionId));
    MOCK_METHOD1(onReceive, void(const std::string& stringOut));
    MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrRuntimeException& error));
};

TEST_P(IltConsumerBroadcastSubscriptionTest, callSubscribeBroadcastWithSinglePrimitiveParameter)
{
    Semaphore publicationSemaphore;
    std::string subscriptionId;
    const std::vector<std::string> partitions = GetParam();

    auto mockBroadcastWithSinglePrimitiveParameterBroadcastListener =
            std::make_shared<MockBroadcastWithSinglePrimitiveParameterBroadcastListener>();
    EXPECT_CALL(*mockBroadcastWithSinglePrimitiveParameterBroadcastListener, onError(_))
            .Times(0)
            .WillRepeatedly(ReleaseSemaphore(&publicationSemaphore));
    EXPECT_CALL(*mockBroadcastWithSinglePrimitiveParameterBroadcastListener, onReceive(Eq("boom")))
            .Times(1)
            .WillRepeatedly(ReleaseSemaphore(&publicationSemaphore));
    std::shared_ptr<ISubscriptionListener<std::string>> listener(
            mockBroadcastWithSinglePrimitiveParameterBroadcastListener);
    JOYNR_ASSERT_NO_THROW({
        JOYNR_LOG_INFO(
                iltConsumerBroadcastSubscriptionTestLogger,
                "callSubscribeBroadcastWithSinglePrimitiveParameter - register subscription");
        testInterfaceProxy->subscribeToBroadcastWithSinglePrimitiveParameterBroadcast(
                                    listener, subscriptionQos, partitions)
                ->get(subscriptionIdFutureTimeoutMs, subscriptionId);

        JOYNR_LOG_INFO(
                iltConsumerBroadcastSubscriptionTestLogger,
                "callSubscribeBroadcastWithSinglePrimitiveParameter - subscription registered");

        JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithSinglePrimitiveParameter - fire broadcast");
        testInterfaceProxy->methodToFireBroadcastWithSinglePrimitiveParameter(partitions);
        ASSERT_TRUE(publicationSemaphore.waitFor(publicationTimeoutMs));

        testInterfaceProxy->unsubscribeFromBroadcastWithSinglePrimitiveParameterBroadcast(
                subscriptionId);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    });
}

class MockBroadcastWithMultiplePrimitiveParametersBroadcastListener
        : public joynr::ISubscriptionListener<double, std::string>
{
public:
    MOCK_METHOD1(onSubscribed, void(const std::string& subscriptionId));
    MOCK_METHOD2(onReceive, void(const double& doubleOut, const std::string& stringOut));
    MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrRuntimeException& error));
};

TEST_P(IltConsumerBroadcastSubscriptionTest, callSubscribeBroadcastWithMultiplePrimitiveParameters)
{
    Semaphore publicationSemaphore;
    double doubleOut;
    std::string subscriptionId;
    const std::vector<std::string> partitions = GetParam();

    auto mockBroadcastWithMultiplePrimitiveParametersBroadcastListener =
            std::make_shared<MockBroadcastWithMultiplePrimitiveParametersBroadcastListener>();
    EXPECT_CALL(*mockBroadcastWithMultiplePrimitiveParametersBroadcastListener, onError(_))
            .Times(0)
            .WillRepeatedly(ReleaseSemaphore(&publicationSemaphore));
    EXPECT_CALL(*mockBroadcastWithMultiplePrimitiveParametersBroadcastListener,
                onReceive(_, Eq("boom")))
            .Times(1)
            .WillRepeatedly(DoAll(SaveArg<0>(&doubleOut), ReleaseSemaphore(&publicationSemaphore)));
    typedef std::shared_ptr<ISubscriptionListener<double, std::string>> listenerType;
    listenerType listener(mockBroadcastWithMultiplePrimitiveParametersBroadcastListener);
    JOYNR_ASSERT_NO_THROW({
        JOYNR_LOG_INFO(
                iltConsumerBroadcastSubscriptionTestLogger,
                "callSubscribeBroadcastWithMultiplePrimitiveParameters - register subscription");
        testInterfaceProxy->subscribeToBroadcastWithMultiplePrimitiveParametersBroadcast(
                                    listener, subscriptionQos, partitions)
                ->get(subscriptionIdFutureTimeoutMs, subscriptionId);

        JOYNR_LOG_INFO(
                iltConsumerBroadcastSubscriptionTestLogger,
                "callSubscribeBroadcastWithMultiplePrimitiveParameters - subscription registered");

        JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithMultiplePrimitiveParameters - fire broadast");
        testInterfaceProxy->methodToFireBroadcastWithMultiplePrimitiveParameters(partitions);
        ASSERT_TRUE(publicationSemaphore.waitFor(publicationTimeoutMs));

        EXPECT_TRUE(IltUtil::cmpDouble(doubleOut, 1.1));

        testInterfaceProxy->unsubscribeFromBroadcastWithMultiplePrimitiveParametersBroadcast(
                subscriptionId);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    });
}

class MockBroadcastWithSingleArrayParameterBroadcastListener
        : public joynr::ISubscriptionListener<std::vector<std::string>>
{
public:
    MOCK_METHOD1(onSubscribed, void(const std::string& subscriptionId));
    MOCK_METHOD1(onReceive, void(const std::vector<std::string>& stringArrayOut));
    MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrRuntimeException& error));
};

TEST_P(IltConsumerBroadcastSubscriptionTest, callSubscribeBroadcastWithSingleArrayParameter)
{
    Semaphore publicationSemaphore;
    std::string subscriptionId;
    std::vector<std::string> stringArrayOut;
    const std::vector<std::string> partitions = GetParam();

    auto mockBroadcastWithSingleArrayParameterBroadcastListener =
            std::make_shared<MockBroadcastWithSingleArrayParameterBroadcastListener>();
    EXPECT_CALL(*mockBroadcastWithSingleArrayParameterBroadcastListener, onError(_))
            .Times(0)
            .WillRepeatedly(ReleaseSemaphore(&publicationSemaphore));
    EXPECT_CALL(*mockBroadcastWithSingleArrayParameterBroadcastListener, onReceive(_))
            .Times(1)
            .WillRepeatedly(
                    DoAll(SaveArg<0>(&stringArrayOut), ReleaseSemaphore(&publicationSemaphore)));
    std::shared_ptr<ISubscriptionListener<std::vector<std::string>>> listener(
            mockBroadcastWithSingleArrayParameterBroadcastListener);
    JOYNR_ASSERT_NO_THROW({
        JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithSingleArrayParameter - register subscription");
        testInterfaceProxy->subscribeToBroadcastWithSingleArrayParameterBroadcast(
                                    listener, subscriptionQos, partitions)
                ->get(subscriptionIdFutureTimeoutMs, subscriptionId);

        JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithSingleArrayParameter - subscription registered");
        JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithSingleArrayParameter - fire broadcast");
        testInterfaceProxy->methodToFireBroadcastWithSingleArrayParameter(partitions);
        ASSERT_TRUE(publicationSemaphore.waitFor(publicationTimeoutMs));

        EXPECT_TRUE(IltUtil::checkStringArray(stringArrayOut));

        testInterfaceProxy->unsubscribeFromBroadcastWithSingleArrayParameterBroadcast(
                subscriptionId);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    });
}

class MockBroadcastWithMultipleArrayParametersBroadcastListener
        : public joynr::ISubscriptionListener<
                  std::vector<uint64_t>,
                  std::vector<
                          joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>>
{
public:
    MOCK_METHOD1(onSubscribed, void(const std::string& subscriptionId));
    MOCK_METHOD2(
            onReceive,
            void(const std::vector<uint64_t>& uInt64ArrayOut,
                 const std::vector<
                         joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>&
                         structWithStringArrayArrayOut));
    MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrRuntimeException& error));
};

TEST_P(IltConsumerBroadcastSubscriptionTest, callSubscribeBroadcastWithMultipleArrayParameters)
{
    Semaphore publicationSemaphore;
    std::string subscriptionId;
    std::vector<uint64_t> uInt64ArrayOut;
    std::vector<joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>
            structWithStringArrayArrayOut;

    const std::vector<std::string> partitions = GetParam();

    auto mockBroadcastWithMultipleArrayParametersBroadcastListener =
            std::make_shared<MockBroadcastWithMultipleArrayParametersBroadcastListener>();
    EXPECT_CALL(*mockBroadcastWithMultipleArrayParametersBroadcastListener, onError(_))
            .Times(0)
            .WillRepeatedly(ReleaseSemaphore(&publicationSemaphore));
    EXPECT_CALL(*mockBroadcastWithMultipleArrayParametersBroadcastListener, onReceive(_, _))
            .Times(1)
            .WillRepeatedly(DoAll(SaveArg<0>(&uInt64ArrayOut),
                                  SaveArg<1>(&structWithStringArrayArrayOut),
                                  ReleaseSemaphore(&publicationSemaphore)));
    typedef std::shared_ptr<ISubscriptionListener<
            std::vector<uint64_t>,
            std::vector<joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>>>
            listenerType;
    listenerType listener(mockBroadcastWithMultipleArrayParametersBroadcastListener);
    JOYNR_ASSERT_NO_THROW({
        JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithMultipleArrayParameters - register subscription");

        testInterfaceProxy->subscribeToBroadcastWithMultipleArrayParametersBroadcast(
                                    listener, subscriptionQos, partitions)
                ->get(subscriptionIdFutureTimeoutMs, subscriptionId);

        JOYNR_LOG_INFO(
                iltConsumerBroadcastSubscriptionTestLogger,
                "callSubscribeBroadcastWithMultipleArrayParameters - subscription registered");

        JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithMultipleArrayParameters - fire broadcast");
        testInterfaceProxy->methodToFireBroadcastWithMultipleArrayParameters(partitions);
        ASSERT_TRUE(publicationSemaphore.waitFor(publicationTimeoutMs));

        EXPECT_TRUE(IltUtil::checkUInt64Array(uInt64ArrayOut));
        EXPECT_TRUE(IltUtil::checkStructWithStringArrayArray(structWithStringArrayArrayOut));

        testInterfaceProxy->unsubscribeFromBroadcastWithMultipleArrayParametersBroadcast(
                subscriptionId);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    });
}

class MockBroadcastWithSingleByteBufferParameterBroadcastListener
        : public joynr::ISubscriptionListener<joynr::ByteBuffer>
{
public:
    MOCK_METHOD1(onSubscribed, void(const std::string& subscriptionId));
    MOCK_METHOD1(onReceive, void(const joynr::ByteBuffer& byteBufferOut));
    MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrRuntimeException& error));
};

TEST_P(IltConsumerBroadcastSubscriptionTest, callSubscribeBroadcastWithSingleByteBufferParameter)
{
    const std::vector<std::string> partitions = GetParam();
    const joynr::ByteBuffer byteBufferOut = {0, 100, 255};

    auto mockBroadcastWithSingleByteBufferParameterBroadcastListener =
            std::make_shared<MockBroadcastWithSingleByteBufferParameterBroadcastListener>();
    joynr::ByteBuffer result;
    Semaphore publicationSemaphore;
    EXPECT_CALL(*mockBroadcastWithSingleByteBufferParameterBroadcastListener, onError(_)).Times(0);
    EXPECT_CALL(*mockBroadcastWithSingleByteBufferParameterBroadcastListener, onReceive(_))
            .WillOnce(DoAll(SaveArg<0>(&result), ReleaseSemaphore(&publicationSemaphore)));

    JOYNR_LOG_DEBUG(iltConsumerBroadcastSubscriptionTestLogger,
                    "callSubscribeBroadcastWithSingleByteBufferParameter - register subscription");

    std::shared_ptr<ISubscriptionListener<joynr::ByteBuffer>> listener(
            mockBroadcastWithSingleByteBufferParameterBroadcastListener);
    std::string subscriptionId;
    JOYNR_ASSERT_NO_THROW(
            testInterfaceProxy->subscribeToBroadcastWithSingleByteBufferParameterBroadcast(
                                        listener, subscriptionQos, partitions)
                    ->get(subscriptionIdFutureTimeoutMs, subscriptionId));

    JOYNR_LOG_DEBUG(
            iltConsumerBroadcastSubscriptionTestLogger,
            "callSubscribeBroadcastWithSingleByteBufferParameter - subscription registered");
    JOYNR_LOG_DEBUG(iltConsumerBroadcastSubscriptionTestLogger,
                    "callSubscribeBroadcastWithSingleByteBufferParameter - fire broadcast");

    JOYNR_ASSERT_NO_THROW(testInterfaceProxy->methodToFireBroadcastWithSingleByteBufferParameter(
            byteBufferOut, partitions));
    ASSERT_TRUE(publicationSemaphore.waitFor(publicationTimeoutMs));

    EXPECT_EQ(result, byteBufferOut);

    JOYNR_ASSERT_NO_THROW(
            testInterfaceProxy->unsubscribeFromBroadcastWithSingleByteBufferParameterBroadcast(
                    subscriptionId));
}

class MockBroadcastWithMultipleByteBufferParametersBroadcastListener
        : public joynr::ISubscriptionListener<joynr::ByteBuffer, joynr::ByteBuffer>
{
public:
    MOCK_METHOD1(onSubscribed, void(const std::string& subscriptionId));
    MOCK_METHOD2(onReceive,
                 void(const joynr::ByteBuffer& byteBufferOut1,
                      const joynr::ByteBuffer& byteBufferOut2));
    MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrRuntimeException& error));
};

TEST_P(IltConsumerBroadcastSubscriptionTest, callSubscribeBroadcastWithMultipleByteBufferParameters)
{
    const std::vector<std::string> partitions = GetParam();
    const joynr::ByteBuffer byteBufferOut1 = {5, 125};
    const joynr::ByteBuffer byteBufferOut2 = {78, 0};

    auto mockBroadcastWithMultipleByteBufferParametersBroadcastListener =
            std::make_shared<MockBroadcastWithMultipleByteBufferParametersBroadcastListener>();
    joynr::ByteBuffer result1;
    joynr::ByteBuffer result2;
    Semaphore publicationSemaphore;
    EXPECT_CALL(*mockBroadcastWithMultipleByteBufferParametersBroadcastListener, onError(_))
            .Times(0);
    EXPECT_CALL(*mockBroadcastWithMultipleByteBufferParametersBroadcastListener, onReceive(_, _))
            .WillOnce(DoAll(SaveArg<0>(&result1),
                            SaveArg<1>(&result2),
                            ReleaseSemaphore(&publicationSemaphore)));

    JOYNR_LOG_DEBUG(
            iltConsumerBroadcastSubscriptionTestLogger,
            "callSubscribeBroadcastWithMultipleByteBufferParameters - register subscription");

    std::shared_ptr<ISubscriptionListener<joynr::ByteBuffer, joynr::ByteBuffer>> listener(
            mockBroadcastWithMultipleByteBufferParametersBroadcastListener);
    std::string subscriptionId;
    JOYNR_ASSERT_NO_THROW(
            testInterfaceProxy->subscribeToBroadcastWithMultipleByteBufferParametersBroadcast(
                                        listener, subscriptionQos, partitions)
                    ->get(subscriptionIdFutureTimeoutMs, subscriptionId));

    JOYNR_LOG_DEBUG(
            iltConsumerBroadcastSubscriptionTestLogger,
            "callSubscribeBroadcastWithMultipleByteBufferParameters - subscription registered");
    JOYNR_LOG_DEBUG(iltConsumerBroadcastSubscriptionTestLogger,
                    "callSubscribeBroadcastWithMultipleByteBufferParameters - fire broadcast");

    JOYNR_ASSERT_NO_THROW(testInterfaceProxy->methodToFireBroadcastWithMultipleByteBufferParameters(
            byteBufferOut1, byteBufferOut2, partitions));
    ASSERT_TRUE(publicationSemaphore.waitFor(publicationTimeoutMs));

    EXPECT_EQ(result1, byteBufferOut1);
    EXPECT_EQ(result2, byteBufferOut2);

    JOYNR_ASSERT_NO_THROW(
            testInterfaceProxy->unsubscribeFromBroadcastWithMultipleByteBufferParametersBroadcast(
                    subscriptionId));
}

class MockBroadcastWithSingleEnumerationParameterBroadcastListener
        : public joynr::ISubscriptionListener<
                  joynr::interlanguagetest::namedTypeCollection2::
                          ExtendedTypeCollectionEnumerationInTypeCollection::Enum>
{
public:
    MOCK_METHOD1(onSubscribed, void(const std::string& subscriptionId));
    MOCK_METHOD1(
            onReceive,
            void(const joynr::interlanguagetest::namedTypeCollection2::
                         ExtendedTypeCollectionEnumerationInTypeCollection::Enum& enumerationOut));
    MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrRuntimeException& error));
};

TEST_P(IltConsumerBroadcastSubscriptionTest, callSubscribeBroadcastWithSingleEnumerationParameter)
{
    Semaphore publicationSemaphore;
    std::string subscriptionId;
    const std::vector<std::string> partitions = GetParam();

    auto mockBroadcastWithSingleEnumerationParameterBroadcastListener =
            std::make_shared<MockBroadcastWithSingleEnumerationParameterBroadcastListener>();
    EXPECT_CALL(*mockBroadcastWithSingleEnumerationParameterBroadcastListener, onError(_))
            .Times(0)
            .WillRepeatedly(ReleaseSemaphore(&publicationSemaphore));
    EXPECT_CALL(*mockBroadcastWithSingleEnumerationParameterBroadcastListener,
                onReceive(Eq(joynr::interlanguagetest::namedTypeCollection2::
                                     ExtendedTypeCollectionEnumerationInTypeCollection::
                                             ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION)))
            .Times(1)
            .WillRepeatedly(ReleaseSemaphore(&publicationSemaphore));
    typedef std::shared_ptr<ISubscriptionListener<
            joynr::interlanguagetest::namedTypeCollection2::
                    ExtendedTypeCollectionEnumerationInTypeCollection::Enum>> listenerType;
    listenerType listener(mockBroadcastWithSingleEnumerationParameterBroadcastListener);
    JOYNR_ASSERT_NO_THROW({
        JOYNR_LOG_INFO(
                iltConsumerBroadcastSubscriptionTestLogger,
                "callSubscribeBroadcastWithSingleEnumerationParameter - register subscription");
        testInterfaceProxy->subscribeToBroadcastWithSingleEnumerationParameterBroadcast(
                                    listener, subscriptionQos, partitions)
                ->get(subscriptionIdFutureTimeoutMs, subscriptionId);

        JOYNR_LOG_INFO(
                iltConsumerBroadcastSubscriptionTestLogger,
                "callSubscribeBroadcastWithSingleEnumerationParameter - subscription registered");

        JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithSingleEnumerationParameter - fire broadast");
        testInterfaceProxy->methodToFireBroadcastWithSingleEnumerationParameter(partitions);
        ASSERT_TRUE(publicationSemaphore.waitFor(publicationTimeoutMs));

        testInterfaceProxy->unsubscribeFromBroadcastWithSingleEnumerationParameterBroadcast(
                subscriptionId);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    });
}

class MockBroadcastWithMultipleEnumerationParametersBroadcastListener
        : public joynr::ISubscriptionListener<
                  joynr::interlanguagetest::namedTypeCollection2::
                          ExtendedEnumerationWithPartlyDefinedValues::Enum,
                  joynr::interlanguagetest::Enumeration::Enum>
{
public:
    MOCK_METHOD1(onSubscribed, void(const std::string& subscriptionId));
    MOCK_METHOD2(
            onReceive,
            void(const joynr::interlanguagetest::namedTypeCollection2::
                         ExtendedEnumerationWithPartlyDefinedValues::Enum& extendedEnumerationOut,
                 const joynr::interlanguagetest::Enumeration::Enum& enumerationOut));
    MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrRuntimeException& error));
};

TEST_P(IltConsumerBroadcastSubscriptionTest,
       callSubscribeBroadcastWithMultipleEnumerationParameters)
{
    Semaphore publicationSemaphore;
    std::string subscriptionId;
    const std::vector<std::string> partitions = GetParam();

    auto mockBroadcastWithMultipleEnumerationParametersBroadcastListener =
            std::make_shared<MockBroadcastWithMultipleEnumerationParametersBroadcastListener>();
    EXPECT_CALL(*mockBroadcastWithMultipleEnumerationParametersBroadcastListener, onError(_))
            .Times(0)
            .WillRepeatedly(ReleaseSemaphore(&publicationSemaphore));
    EXPECT_CALL(
            *mockBroadcastWithMultipleEnumerationParametersBroadcastListener,
            onReceive(Eq(joynr::interlanguagetest::namedTypeCollection2::
                                 ExtendedEnumerationWithPartlyDefinedValues::
                                         ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES),
                      Eq(joynr::interlanguagetest::Enumeration::ENUM_0_VALUE_1)))
            .Times(1)
            .WillRepeatedly(ReleaseSemaphore(&publicationSemaphore));
    typedef std::shared_ptr<
            ISubscriptionListener<joynr::interlanguagetest::namedTypeCollection2::
                                          ExtendedEnumerationWithPartlyDefinedValues::Enum,
                                  joynr::interlanguagetest::Enumeration::Enum>> listenerType;
    listenerType listener(mockBroadcastWithMultipleEnumerationParametersBroadcastListener);
    JOYNR_ASSERT_NO_THROW({
        JOYNR_LOG_INFO(
                iltConsumerBroadcastSubscriptionTestLogger,
                "callSubscribeBroadcastWithMultipleEnumerationParameters - register subscription");
        testInterfaceProxy->subscribeToBroadcastWithMultipleEnumerationParametersBroadcast(
                                    listener, subscriptionQos, partitions)
                ->get(subscriptionIdFutureTimeoutMs, subscriptionId);

        JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithMulti"
                       "pleEnumerationParameters - "
                       "subscription registered");

        JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithMultipleEnumerationParameters - fire broadast");
        testInterfaceProxy->methodToFireBroadcastWithMultipleEnumerationParameters(partitions);
        ASSERT_TRUE(publicationSemaphore.waitFor(publicationTimeoutMs));

        testInterfaceProxy->unsubscribeFromBroadcastWithMultipleEnumerationParametersBroadcast(
                subscriptionId);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    });
}

class MockBroadcastWithSingleStructParameterBroadcastListener
        : public joynr::ISubscriptionListener<
                  joynr::interlanguagetest::namedTypeCollection2::ExtendedStructOfPrimitives>
{
public:
    MOCK_METHOD1(onSubscribed, void(const std::string& subscriptionId));
    MOCK_METHOD1(
            onReceive,
            void(const joynr::interlanguagetest::namedTypeCollection2::ExtendedStructOfPrimitives&
                         extendedStructOfPrimitivesOut));
    MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrRuntimeException& error));
};

TEST_P(IltConsumerBroadcastSubscriptionTest, callSubscribeBroadcastWithSingleStructParameter)
{
    Semaphore publicationSemaphore;
    joynr::interlanguagetest::namedTypeCollection2::ExtendedStructOfPrimitives
            extendedStructOfPrimitivesOut;
    std::string subscriptionId;

    const std::vector<std::string> partitions = GetParam();

    auto mockBroadcastWithSingleStructParameterBroadcastListener =
            std::make_shared<MockBroadcastWithSingleStructParameterBroadcastListener>();
    EXPECT_CALL(*mockBroadcastWithSingleStructParameterBroadcastListener, onError(_))
            .Times(0)
            .WillRepeatedly(ReleaseSemaphore(&publicationSemaphore));
    EXPECT_CALL(*mockBroadcastWithSingleStructParameterBroadcastListener, onReceive(_))
            .Times(1)
            .WillRepeatedly(DoAll(SaveArg<0>(&extendedStructOfPrimitivesOut),
                                  ReleaseSemaphore(&publicationSemaphore)));
    typedef std::shared_ptr<ISubscriptionListener<
            joynr::interlanguagetest::namedTypeCollection2::ExtendedStructOfPrimitives>>
            listenerType;
    listenerType listener(mockBroadcastWithSingleStructParameterBroadcastListener);
    JOYNR_ASSERT_NO_THROW({
        JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithSingleStructParameter - register subscription");
        testInterfaceProxy->subscribeToBroadcastWithSingleStructParameterBroadcast(
                                    listener, subscriptionQos, partitions)
                ->get(subscriptionIdFutureTimeoutMs, subscriptionId);

        JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithSingleStructParameter - subscription registered");

        JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithSingleStructParameter - fire broadast");
        testInterfaceProxy->methodToFireBroadcastWithSingleStructParameter(partitions);
        ASSERT_TRUE(publicationSemaphore.waitFor(publicationTimeoutMs));

        EXPECT_TRUE(IltUtil::checkExtendedStructOfPrimitives(extendedStructOfPrimitivesOut));

        testInterfaceProxy->unsubscribeFromBroadcastWithSingleStructParameterBroadcast(
                subscriptionId);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    });
}

class MockBroadcastWithMultipleStructParametersBroadcastListener
        : public joynr::ISubscriptionListener<
                  joynr::interlanguagetest::namedTypeCollection2::BaseStructWithoutElements,
                  joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedBaseStruct>
{
public:
    MOCK_METHOD1(onSubscribed, void(const std::string& subscriptionId));
    MOCK_METHOD2(
            onReceive,
            void(const joynr::interlanguagetest::namedTypeCollection2::BaseStructWithoutElements&
                         baseStructWithoutElementsOut,
                 const joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedBaseStruct&
                         extendedExtendedBaseStructOut));
    MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrRuntimeException& error));
};

TEST_P(IltConsumerBroadcastSubscriptionTest, callSubscribeBroadcastWithMultipleStructParameters)
{
    Semaphore publicationSemaphore;
    joynr::interlanguagetest::namedTypeCollection2::BaseStructWithoutElements
            baseStructWithoutElementsOut;
    joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedBaseStruct
            extendedExtendedBaseStructOut;
    std::string subscriptionId;

    const std::vector<std::string> partitions = GetParam();

    auto mockBroadcastWithMultipleStructParametersBroadcastListener =
            std::make_shared<MockBroadcastWithMultipleStructParametersBroadcastListener>();
    EXPECT_CALL(*mockBroadcastWithMultipleStructParametersBroadcastListener, onError(_))
            .Times(0)
            .WillRepeatedly(ReleaseSemaphore(&publicationSemaphore));
    EXPECT_CALL(*mockBroadcastWithMultipleStructParametersBroadcastListener, onReceive(_, _))
            .Times(1)
            .WillRepeatedly(DoAll(SaveArg<0>(&baseStructWithoutElementsOut),
                                  SaveArg<1>(&extendedExtendedBaseStructOut),
                                  ReleaseSemaphore(&publicationSemaphore)));
    typedef std::shared_ptr<ISubscriptionListener<
            joynr::interlanguagetest::namedTypeCollection2::BaseStructWithoutElements,
            joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedBaseStruct>>
            listenerType;
    listenerType listener(mockBroadcastWithMultipleStructParametersBroadcastListener);
    JOYNR_ASSERT_NO_THROW({
        JOYNR_LOG_INFO(
                iltConsumerBroadcastSubscriptionTestLogger,
                "callSubscribeBroadcastWithMultipleStructParameters - register subscription");
        testInterfaceProxy->subscribeToBroadcastWithMultipleStructParametersBroadcast(
                                    listener, subscriptionQos, partitions)
                ->get(subscriptionIdFutureTimeoutMs, subscriptionId);

        JOYNR_LOG_INFO(
                iltConsumerBroadcastSubscriptionTestLogger,
                "callSubscribeBroadcastWithMultipleStructParameters - subscription registered");

        JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithMultipleStructParameters - fire broadast");
        testInterfaceProxy->methodToFireBroadcastWithMultipleStructParameters(partitions);
        ASSERT_TRUE(publicationSemaphore.waitFor(publicationTimeoutMs));

        EXPECT_TRUE(IltUtil::checkBaseStructWithoutElements(baseStructWithoutElementsOut));
        EXPECT_TRUE(IltUtil::checkExtendedExtendedBaseStruct(extendedExtendedBaseStructOut));

        testInterfaceProxy->unsubscribeFromBroadcastWithMultipleStructParametersBroadcast(
                subscriptionId);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    });
}

TEST_F(IltConsumerBroadcastSubscriptionTest, doNotReceivePublicationsForOtherPartitions)
{
    Semaphore publicationSemaphore;

    auto mockBroadcastWithSingleEnumerationParameter =
            std::make_shared<MockBroadcastWithSingleEnumerationParameterBroadcastListener>();

    EXPECT_CALL(*mockBroadcastWithSingleEnumerationParameter, onReceive(_)).Times(1).WillRepeatedly(
            ReleaseSemaphore(&publicationSemaphore));

    const std::vector<std::string> subscribeToPartitions{"partition0", "partition1"};
    const std::vector<std::string> broadcastPartitions{"otherPartition"};

    JOYNR_ASSERT_NO_THROW({
        std::string subscriptionId;
        auto subscriptionIdFuture =
                testInterfaceProxy->subscribeToBroadcastWithSingleEnumerationParameterBroadcast(
                        mockBroadcastWithSingleEnumerationParameter,
                        subscriptionQos,
                        subscribeToPartitions);

        subscriptionIdFuture->get(subscriptionIdFutureTimeoutMs, subscriptionId);

        testInterfaceProxy->methodToFireBroadcastWithSingleEnumerationParameter(
                broadcastPartitions);

        // No broadcast shall be received because the partitions do not match.
        ASSERT_FALSE(publicationSemaphore.waitFor(std::chrono::milliseconds(2000)));

        // Ensure that we did not receive a publication for another reasons
        testInterfaceProxy->methodToFireBroadcastWithSingleEnumerationParameter(
                subscribeToPartitions);

        ASSERT_TRUE(publicationSemaphore.waitFor(publicationTimeoutMs));

        testInterfaceProxy->unsubscribeFromBroadcastWithSingleEnumerationParameterBroadcast(
                subscriptionId);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    });
}

INSTANTIATE_TEST_SUITE_P(NoPartitions,
                         IltConsumerBroadcastSubscriptionTest,
                         ::testing::Values(std::vector<std::string>()));

INSTANTIATE_TEST_SUITE_P(WithPartitions,
                         IltConsumerBroadcastSubscriptionTest,
                         ::testing::Values(std::vector<std::string>({"partition0", "partition1"})));
