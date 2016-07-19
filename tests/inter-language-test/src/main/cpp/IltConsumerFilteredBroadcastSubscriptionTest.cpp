/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
#include "joynr/SubscriptionListener.h"
#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/interlanguagetest/TestInterfaceBroadcastWithFilteringBroadcastFilterParameters.h"

using namespace ::testing;

class IltConsumerFilteredBroadcastSubscriptionTest : public IltAbstractConsumerTest
{
public:
    IltConsumerFilteredBroadcastSubscriptionTest() = default;

    static volatile bool subscribeBroadcastWithFilteringCallbackDone;
    static volatile bool subscribeBroadcastWithFilteringCallbackResult;
};

joynr::Logger iltConsumerFilteredBroadcastSubscriptionTestLogger(
        "IltConsumerFilteredBroadcastSubscriptionTest");

// variables that are to be changed inside callbacks must be declared global
volatile bool
        IltConsumerFilteredBroadcastSubscriptionTest::subscribeBroadcastWithFilteringCallbackDone =
                false;
volatile bool IltConsumerFilteredBroadcastSubscriptionTest::
        subscribeBroadcastWithFilteringCallbackResult = false;

class BroadcastWithFilteringBroadcastListener
        : public SubscriptionListener<
                  std::string,
                  std::vector<std::string>,
                  joynr::interlanguagetest::namedTypeCollection2::
                          ExtendedTypeCollectionEnumerationInTypeCollection::Enum,
                  joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray,
                  std::vector<
                          joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>>
{
public:
    BroadcastWithFilteringBroadcastListener()
    {
    }

    ~BroadcastWithFilteringBroadcastListener()
    {
    }

    void onReceive(const std::string& stringOut,
                   const std::vector<std::string>& stringArrayOut,
                   const joynr::interlanguagetest::namedTypeCollection2::
                           ExtendedTypeCollectionEnumerationInTypeCollection::Enum& enumerationOut,
                   const joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray&
                           structWithStringArrayOut,
                   const std::vector<
                           joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>&
                           structWithStringArrayArrayOut)
    {
        JOYNR_LOG_INFO(iltConsumerFilteredBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithFiltering - callback - got broadcast");
        if (!IltUtil::checkStringArray(stringArrayOut)) {
            JOYNR_LOG_INFO(iltConsumerFilteredBroadcastSubscriptionTestLogger,
                           "callSubscribeBroadcastWithFiltering - callback - invalid "
                           "stringArrayOut content");
            IltConsumerFilteredBroadcastSubscriptionTest::
                    subscribeBroadcastWithFilteringCallbackResult = false;
        } else if (enumerationOut != joynr::interlanguagetest::namedTypeCollection2::
                                             ExtendedTypeCollectionEnumerationInTypeCollection::
                                                     ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION) {
            JOYNR_LOG_INFO(iltConsumerFilteredBroadcastSubscriptionTestLogger,
                           "callSubscribeBroadcastWithFiltering - callback - invalid "
                           "enumerationOut content");
            IltConsumerFilteredBroadcastSubscriptionTest::
                    subscribeBroadcastWithFilteringCallbackResult = false;
        } else if (!IltUtil::checkStructWithStringArray(structWithStringArrayOut)) {
            JOYNR_LOG_INFO(iltConsumerFilteredBroadcastSubscriptionTestLogger,
                           "callSubscribeBroadcastWithFiltering - callback - invalid "
                           "structWithStringArrayOut content");
            IltConsumerFilteredBroadcastSubscriptionTest::
                    subscribeBroadcastWithFilteringCallbackResult = false;
        } else if (!IltUtil::checkStructWithStringArrayArray(structWithStringArrayArrayOut)) {
            JOYNR_LOG_INFO(iltConsumerFilteredBroadcastSubscriptionTestLogger,
                           "callSubscribeBroadcastWithFiltering - callback - invalid "
                           "structWithStringArrayArrayOut content");
            IltConsumerFilteredBroadcastSubscriptionTest::
                    subscribeBroadcastWithFilteringCallbackResult = false;
        } else {
            JOYNR_LOG_INFO(iltConsumerFilteredBroadcastSubscriptionTestLogger,
                           "callSubscribeBroadcastWithFiltering - callback - content "
                           "OK");
            IltConsumerFilteredBroadcastSubscriptionTest::
                    subscribeBroadcastWithFilteringCallbackResult = true;
        }
        IltConsumerFilteredBroadcastSubscriptionTest::subscribeBroadcastWithFilteringCallbackDone =
                true;
    }

    void onError(const joynr::exceptions::JoynrRuntimeException& error)
    {
        JOYNR_LOG_INFO(iltConsumerFilteredBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithFiltering - callback - got error");
        IltConsumerFilteredBroadcastSubscriptionTest::
                subscribeBroadcastWithFilteringCallbackResult = false;
        IltConsumerFilteredBroadcastSubscriptionTest::subscribeBroadcastWithFilteringCallbackDone =
                true;
    }
};

TEST_F(IltConsumerFilteredBroadcastSubscriptionTest, callSubscribeBroadcastWithFiltering)
{
    std::string subscriptionId;
    int64_t minInterval_ms = 0;
    int64_t validity = 60000;
    joynr::OnChangeSubscriptionQos subscriptionQos(validity, minInterval_ms);
    bool result;
    typedef std::shared_ptr<ISubscriptionListener<
            std::string,
            std::vector<std::string>,
            joynr::interlanguagetest::namedTypeCollection2::
                    ExtendedTypeCollectionEnumerationInTypeCollection::Enum,
            joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray,
            std::vector<joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>>>
            listenerType;
    JOYNR_ASSERT_NO_THROW({
        listenerType listener(new BroadcastWithFilteringBroadcastListener());
        joynr::interlanguagetest::TestInterfaceBroadcastWithFilteringBroadcastFilterParameters
                filterParameters;

        std::string filterStringOfInterest = "fireBroadcast";
        std::vector<std::string> filterStringArrayOfInterest = IltUtil::createStringArray();
        std::string filterStringArrayOfInterestJson(
                JsonSerializer::serialize(filterStringArrayOfInterest));

        joynr::interlanguagetest::namedTypeCollection2::
                ExtendedTypeCollectionEnumerationInTypeCollection::Enum filterEnumOfInterest =
                        joynr::interlanguagetest::namedTypeCollection2::
                                ExtendedTypeCollectionEnumerationInTypeCollection::
                                        ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION;
        std::string filterEnumOfInterestJson(
                "\"" + joynr::interlanguagetest::namedTypeCollection2::
                               ExtendedTypeCollectionEnumerationInTypeCollection::getLiteral(
                                       filterEnumOfInterest) +
                "\"");

        joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray
                filterStructWithStringArray = IltUtil::createStructWithStringArray();
        std::string filterStructWithStringArrayJson(
                JsonSerializer::serialize(filterStructWithStringArray));

        std::vector<joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>
                filterStructWithStringArrayArray = IltUtil::createStructWithStringArrayArray();
        std::string filterStructWithStringArrayArrayJson(
                JsonSerializer::serialize(filterStructWithStringArrayArray));

        filterParameters.setStringOfInterest(filterStringOfInterest);
        filterParameters.setStringArrayOfInterest(filterStringArrayOfInterestJson);
        filterParameters.setEnumerationOfInterest(filterEnumOfInterestJson);
        filterParameters.setStructWithStringArrayOfInterest(filterStructWithStringArrayJson);
        filterParameters.setStructWithStringArrayArrayOfInterest(
                filterStructWithStringArrayArrayJson);

        subscriptionId = testInterfaceProxy->subscribeToBroadcastWithFilteringBroadcast(
                filterParameters, listener, subscriptionQos);
        usleep(1000000);
        std::string stringArg = "fireBroadcast";
        testInterfaceProxy->methodToFireBroadcastWithFiltering(stringArg);
        waitForChange(subscribeBroadcastWithFilteringCallbackDone, 1000);
        ASSERT_TRUE(subscribeBroadcastWithFilteringCallbackDone);
        ASSERT_TRUE(subscribeBroadcastWithFilteringCallbackResult);

        // reset counter for 2nd test
        subscribeBroadcastWithFilteringCallbackDone = false;
        subscribeBroadcastWithFilteringCallbackResult = false;

        stringArg = "doNotFireBroadcast";
        testInterfaceProxy->methodToFireBroadcastWithFiltering(stringArg);
        waitForChange(subscribeBroadcastWithFilteringCallbackDone, 1000);
        ASSERT_FALSE(subscribeBroadcastWithFilteringCallbackDone);

        testInterfaceProxy->unsubscribeFromBroadcastWithFilteringBroadcast(subscriptionId);
    });
}
