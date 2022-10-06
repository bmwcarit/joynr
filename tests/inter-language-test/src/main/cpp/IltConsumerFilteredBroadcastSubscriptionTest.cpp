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
#include "IltAbstractConsumerTest.h"
#include "joynr/ISubscriptionListener.h"
#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/Semaphore.h"
#include "joynr/SubscriptionListener.h"
#include "joynr/interlanguagetest/TestInterfaceBroadcastWithFilteringBroadcastFilterParameters.h"
#include "joynr/serializer/Serializer.h"

using namespace ::testing;

class IltConsumerFilteredBroadcastSubscriptionTest : public IltAbstractConsumerTest<::testing::Test>
{
public:
    IltConsumerFilteredBroadcastSubscriptionTest() = default;
};

joynr::Logger iltConsumerFilteredBroadcastSubscriptionTestLogger(
        "IltConsumerFilteredBroadcastSubscriptionTest");

class MockBroadcastWithFilteringBroadcastListener
        : public joynr::ISubscriptionListener<
                  std::string,
                  std::vector<std::string>,
                  joynr::interlanguagetest::namedTypeCollection2::
                          ExtendedTypeCollectionEnumerationInTypeCollection::Enum,
                  joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray,
                  std::vector<
                          joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>>
{
public:
    MOCK_METHOD1(onSubscribed, void(const std::string& subscriptionId));
    MOCK_METHOD5(
            onReceive,
            void(const std::string& stringOut,
                 const std::vector<std::string>& stringArrayOut,
                 const joynr::interlanguagetest::namedTypeCollection2::
                         ExtendedTypeCollectionEnumerationInTypeCollection::Enum& enumerationOut,
                 const joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray&
                         structWithStringArrayOut,
                 const std::vector<
                         joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>&
                         structWithStringArrayArrayOut));
    MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrRuntimeException& error));
};

TEST_F(IltConsumerFilteredBroadcastSubscriptionTest, callSubscribeBroadcastWithFiltering)
{
    Semaphore publicationSemaphore;
    std::vector<std::string> stringArrayOut;
    joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray structWithStringArrayOut;
    std::vector<joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>
            structWithStringArrayArrayOut;
    std::string subscriptionId;

    auto mockBroadcastWithFilteringBroadcastListener =
            std::make_shared<MockBroadcastWithFilteringBroadcastListener>();
    EXPECT_CALL(*mockBroadcastWithFilteringBroadcastListener, onError(_))
            .Times(0)
            .WillRepeatedly(ReleaseSemaphore(&publicationSemaphore));
    EXPECT_CALL(*mockBroadcastWithFilteringBroadcastListener,
                onReceive(Eq("fireBroadcast"),
                          _,
                          Eq(joynr::interlanguagetest::namedTypeCollection2::
                                     ExtendedTypeCollectionEnumerationInTypeCollection::
                                             ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION),
                          _,
                          _))
            .Times(1)
            .WillRepeatedly(DoAll(SaveArg<1>(&stringArrayOut),
                                  SaveArg<3>(&structWithStringArrayOut),
                                  SaveArg<4>(&structWithStringArrayArrayOut),
                                  ReleaseSemaphore(&publicationSemaphore)));
    EXPECT_CALL(*mockBroadcastWithFilteringBroadcastListener,
                onReceive(Eq("doNotFireBroadcast"),
                          _,
                          Eq(joynr::interlanguagetest::namedTypeCollection2::
                                     ExtendedTypeCollectionEnumerationInTypeCollection::
                                             ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION),
                          _,
                          _))
            .Times(0)
            .WillRepeatedly(DoAll(SaveArg<1>(&stringArrayOut),
                                  SaveArg<3>(&structWithStringArrayOut),
                                  SaveArg<4>(&structWithStringArrayArrayOut),
                                  ReleaseSemaphore(&publicationSemaphore)));
    typedef std::shared_ptr<ISubscriptionListener<
            std::string,
            std::vector<std::string>,
            joynr::interlanguagetest::namedTypeCollection2::
                    ExtendedTypeCollectionEnumerationInTypeCollection::Enum,
            joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray,
            std::vector<joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>>>
            listenerType;
    listenerType listener(mockBroadcastWithFilteringBroadcastListener);
    JOYNR_ASSERT_NO_THROW({
        joynr::interlanguagetest::TestInterfaceBroadcastWithFilteringBroadcastFilterParameters
                filterParameters;

        std::string filterStringOfInterest = "fireBroadcast";
        std::vector<std::string> filterStringArrayOfInterest = IltUtil::createStringArray();
        std::string filterStringArrayOfInterestJson(
                joynr::serializer::serializeToJson(filterStringArrayOfInterest));

        joynr::interlanguagetest::namedTypeCollection2::
                ExtendedTypeCollectionEnumerationInTypeCollection::Enum filterEnumOfInterest =
                        joynr::interlanguagetest::namedTypeCollection2::
                                ExtendedTypeCollectionEnumerationInTypeCollection::
                                        ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION;
        std::string filterEnumOfInterestJson(
                "\"" +
                joynr::interlanguagetest::namedTypeCollection2::
                        ExtendedTypeCollectionEnumerationInTypeCollection::getLiteral(
                                filterEnumOfInterest) +
                "\"");

        joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray
                filterStructWithStringArray = IltUtil::createStructWithStringArray();
        std::string filterStructWithStringArrayJson(
                joynr::serializer::serializeToJson(filterStructWithStringArray));

        std::vector<joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>
                filterStructWithStringArrayArray = IltUtil::createStructWithStringArrayArray();
        std::string filterStructWithStringArrayArrayJson(
                joynr::serializer::serializeToJson(filterStructWithStringArrayArray));

        filterParameters.setStringOfInterest(filterStringOfInterest);
        filterParameters.setStringArrayOfInterest(filterStringArrayOfInterestJson);
        filterParameters.setEnumerationOfInterest(filterEnumOfInterestJson);
        filterParameters.setStructWithStringArrayOfInterest(filterStructWithStringArrayJson);
        filterParameters.setStructWithStringArrayArrayOfInterest(
                filterStructWithStringArrayArrayJson);

        JOYNR_LOG_INFO(iltConsumerFilteredBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithFiltering - register subscription");

        int64_t minInterval_ms = 0;
        int64_t validity = 60000;
        int64_t publicationTtl = UnicastSubscriptionQos::DEFAULT_PUBLICATION_TTL_MS();
        auto subscriptionQos = std::make_shared<joynr::OnChangeSubscriptionQos>(
                validity, publicationTtl, minInterval_ms);
        testInterfaceProxy
                ->subscribeToBroadcastWithFilteringBroadcast(
                        filterParameters, listener, subscriptionQos)
                ->get(subscriptionIdFutureTimeoutMs, subscriptionId);

        JOYNR_LOG_INFO(iltConsumerFilteredBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithFiltering - subscription registered");

        JOYNR_LOG_INFO(iltConsumerFilteredBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithFiltering - fire broadast \"fireBroadcast\"");
        std::string stringArg = "fireBroadcast";
        testInterfaceProxy->methodToFireBroadcastWithFiltering(stringArg);
        ASSERT_TRUE(publicationSemaphore.waitFor(publicationTimeoutMs));

        EXPECT_TRUE(IltUtil::checkStringArray(stringArrayOut));
        EXPECT_TRUE(IltUtil::checkStructWithStringArray(structWithStringArrayOut));
        EXPECT_TRUE(IltUtil::checkStructWithStringArrayArray(structWithStringArrayArrayOut));

        JOYNR_LOG_INFO(
                iltConsumerFilteredBroadcastSubscriptionTestLogger,
                "callSubscribeBroadcastWithFiltering - fire broadast \"doNotFireBroadcast\"");
        stringArg = "doNotFireBroadcast";
        testInterfaceProxy->methodToFireBroadcastWithFiltering(stringArg);
        ASSERT_FALSE(publicationSemaphore.waitFor(publicationTimeoutMs));

        testInterfaceProxy->unsubscribeFromBroadcastWithFilteringBroadcast(subscriptionId);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    });
}
