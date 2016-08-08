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

using namespace ::testing;

class IltConsumerBroadcastSubscriptionTest : public IltAbstractConsumerTest
{
public:
    IltConsumerBroadcastSubscriptionTest() = default;

    static volatile bool subscribeBroadcastWithSinglePrimitiveParameterCallbackDone;
    static volatile bool subscribeBroadcastWithSinglePrimitiveParameterCallbackResult;
    static volatile bool subscribeBroadcastWithMultiplePrimitiveParametersCallbackDone;
    static volatile bool subscribeBroadcastWithMultiplePrimitiveParametersCallbackResult;
    static volatile bool subscribeBroadcastWithSingleArrayParameterCallbackDone;
    static volatile bool subscribeBroadcastWithSingleArrayParameterCallbackResult;
    static volatile bool subscribeBroadcastWithMultipleArrayParametersCallbackDone;
    static volatile bool subscribeBroadcastWithMultipleArrayParametersCallbackResult;
    static volatile bool subscribeBroadcastWithSingleEnumerationParameterCallbackDone;
    static volatile bool subscribeBroadcastWithSingleEnumerationParameterCallbackResult;
    static volatile bool subscribeBroadcastWithMultipleEnumerationParametersCallbackDone;
    static volatile bool subscribeBroadcastWithMultipleEnumerationParametersCallbackResult;
    static volatile bool subscribeBroadcastWithSingleStructParameterCallbackDone;
    static volatile bool subscribeBroadcastWithSingleStructParameterCallbackResult;
    static volatile bool subscribeBroadcastWithMultipleStructParametersCallbackDone;
    static volatile bool subscribeBroadcastWithMultipleStructParametersCallbackResult;
};

joynr::Logger iltConsumerBroadcastSubscriptionTestLogger("IltConsumerBroadcastSubscriptionTest");

volatile bool IltConsumerBroadcastSubscriptionTest::
        subscribeBroadcastWithSinglePrimitiveParameterCallbackDone = false;
volatile bool IltConsumerBroadcastSubscriptionTest::
        subscribeBroadcastWithSinglePrimitiveParameterCallbackResult = false;
volatile bool IltConsumerBroadcastSubscriptionTest::
        subscribeBroadcastWithMultiplePrimitiveParametersCallbackDone = false;
volatile bool IltConsumerBroadcastSubscriptionTest::
        subscribeBroadcastWithMultiplePrimitiveParametersCallbackResult = false;
volatile bool IltConsumerBroadcastSubscriptionTest::
        subscribeBroadcastWithSingleArrayParameterCallbackDone = false;
volatile bool IltConsumerBroadcastSubscriptionTest::
        subscribeBroadcastWithSingleArrayParameterCallbackResult = false;
volatile bool IltConsumerBroadcastSubscriptionTest::
        subscribeBroadcastWithMultipleArrayParametersCallbackDone = false;
volatile bool IltConsumerBroadcastSubscriptionTest::
        subscribeBroadcastWithMultipleArrayParametersCallbackResult = false;
volatile bool IltConsumerBroadcastSubscriptionTest::
        subscribeBroadcastWithSingleEnumerationParameterCallbackDone = false;
volatile bool IltConsumerBroadcastSubscriptionTest::
        subscribeBroadcastWithSingleEnumerationParameterCallbackResult = false;
volatile bool IltConsumerBroadcastSubscriptionTest::
        subscribeBroadcastWithMultipleEnumerationParametersCallbackDone = false;
volatile bool IltConsumerBroadcastSubscriptionTest::
        subscribeBroadcastWithMultipleEnumerationParametersCallbackResult = false;
volatile bool IltConsumerBroadcastSubscriptionTest::
        subscribeBroadcastWithSingleStructParameterCallbackDone = false;
volatile bool IltConsumerBroadcastSubscriptionTest::
        subscribeBroadcastWithSingleStructParameterCallbackResult = false;
volatile bool IltConsumerBroadcastSubscriptionTest::
        subscribeBroadcastWithMultipleStructParametersCallbackDone = false;
volatile bool IltConsumerBroadcastSubscriptionTest::
        subscribeBroadcastWithMultipleStructParametersCallbackResult = false;

class BroadcastWithSinglePrimitiveParameterBroadcastListener
        : public SubscriptionListener<std::string>
{
public:
    BroadcastWithSinglePrimitiveParameterBroadcastListener()
    {
    }

    ~BroadcastWithSinglePrimitiveParameterBroadcastListener()
    {
    }

    void onReceive(const std::string& stringOut)
    {
        JOYNR_LOG_INFO(
                iltConsumerBroadcastSubscriptionTestLogger,
                "callSubscribeBroadcastWithSinglePrimitiveParameter - callback - got broadcast");
        if (stringOut != "boom") {
            JOYNR_LOG_INFO(
                    iltConsumerBroadcastSubscriptionTestLogger,
                    "callSubscribeBroadcastWithSinglePrimitiveParameter - callback - invalid "
                    "content");
            IltConsumerBroadcastSubscriptionTest::
                    subscribeBroadcastWithSinglePrimitiveParameterCallbackResult = false;
        } else {
            JOYNR_LOG_INFO(
                    iltConsumerBroadcastSubscriptionTestLogger,
                    "callSubscribeBroadcastWithSinglePrimitiveParameter - callback - content OK");
            IltConsumerBroadcastSubscriptionTest::
                    subscribeBroadcastWithSinglePrimitiveParameterCallbackResult = true;
        }
        IltConsumerBroadcastSubscriptionTest::
                subscribeBroadcastWithSinglePrimitiveParameterCallbackDone = true;
    }

    void onError(const joynr::exceptions::JoynrRuntimeException& error)
    {
        JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithSinglePrimitiveParameter - callback - got error");
        IltConsumerBroadcastSubscriptionTest::
                subscribeBroadcastWithSinglePrimitiveParameterCallbackResult = false;
        IltConsumerBroadcastSubscriptionTest::
                subscribeBroadcastWithSinglePrimitiveParameterCallbackDone = true;
    }
};

TEST_F(IltConsumerBroadcastSubscriptionTest, callSubscribeBroadcastWithSinglePrimitiveParameter)
{
    std::string subscriptionId;
    int64_t minInterval_ms = 0;
    int64_t validity = 60000;
    auto subscriptionQos =
            std::make_shared<joynr::OnChangeSubscriptionQos>(validity, minInterval_ms);
    JOYNR_ASSERT_NO_THROW({
        std::shared_ptr<ISubscriptionListener<std::string>> listener(
                new BroadcastWithSinglePrimitiveParameterBroadcastListener());
        subscriptionId =
                testInterfaceProxy->subscribeToBroadcastWithSinglePrimitiveParameterBroadcast(
                        listener, subscriptionQos);
        usleep(1000000);
        testInterfaceProxy->methodToFireBroadcastWithSinglePrimitiveParameter();
        waitForChange(subscribeBroadcastWithSinglePrimitiveParameterCallbackDone, 1000);
        ASSERT_TRUE(subscribeBroadcastWithSinglePrimitiveParameterCallbackDone);
        ASSERT_TRUE(subscribeBroadcastWithSinglePrimitiveParameterCallbackResult);

        testInterfaceProxy->unsubscribeFromBroadcastWithSinglePrimitiveParameterBroadcast(
                subscriptionId);
    });
}

class BroadcastWithMultiplePrimitiveParametersBroadcastListener
        : public SubscriptionListener<double, std::string>
{
public:
    BroadcastWithMultiplePrimitiveParametersBroadcastListener()
    {
    }

    ~BroadcastWithMultiplePrimitiveParametersBroadcastListener()
    {
    }

    void onReceive(const double& doubleOut, const std::string& stringOut)
    {
        JOYNR_LOG_INFO(
                iltConsumerBroadcastSubscriptionTestLogger,
                "callSubscribeBroadcastWithMultiplePrimitiveParameters - callback - got broadcast");
        if (!IltUtil::cmpDouble(doubleOut, 1.1)) {
            JOYNR_LOG_INFO(
                    iltConsumerBroadcastSubscriptionTestLogger,
                    "callSubscribeBroadcastWithMultiplePrimitiveParameters - callback - invalid "
                    "doubleOut content");
            IltConsumerBroadcastSubscriptionTest::
                    subscribeBroadcastWithMultiplePrimitiveParametersCallbackResult = false;
        } else if (stringOut != "boom") {
            JOYNR_LOG_INFO(
                    iltConsumerBroadcastSubscriptionTestLogger,
                    "callSubscribeBroadcastWithMultiplePrimitiveParameters - callback - invalid "
                    "content");
            IltConsumerBroadcastSubscriptionTest::
                    subscribeBroadcastWithMultiplePrimitiveParametersCallbackResult = false;
        } else {
            JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                           "callSubscribeBroadcastWithMultiplePrimitiveParameters - "
                           "callback - content OK");
            IltConsumerBroadcastSubscriptionTest::
                    subscribeBroadcastWithMultiplePrimitiveParametersCallbackResult = true;
        }
        IltConsumerBroadcastSubscriptionTest::
                subscribeBroadcastWithMultiplePrimitiveParametersCallbackDone = true;
    }

    void onError(const joynr::exceptions::JoynrRuntimeException& error)
    {
        JOYNR_LOG_INFO(
                iltConsumerBroadcastSubscriptionTestLogger,
                "callSubscribeBroadcastWithMultiplePrimitiveParameters - callback - got error");
        IltConsumerBroadcastSubscriptionTest::
                subscribeBroadcastWithMultiplePrimitiveParametersCallbackResult = false;
        IltConsumerBroadcastSubscriptionTest::
                subscribeBroadcastWithMultiplePrimitiveParametersCallbackDone = true;
    }
};

TEST_F(IltConsumerBroadcastSubscriptionTest, callSubscribeBroadcastWithMultiplePrimitiveParameters)
{
    std::string subscriptionId;
    int64_t minInterval_ms = 0;
    int64_t validity = 60000;
    auto subscriptionQos =
            std::make_shared<joynr::OnChangeSubscriptionQos>(validity, minInterval_ms);
    typedef std::shared_ptr<ISubscriptionListener<double, std::string>> listenerType;
    JOYNR_ASSERT_NO_THROW({
        listenerType listener(new BroadcastWithMultiplePrimitiveParametersBroadcastListener());
        subscriptionId =
                testInterfaceProxy->subscribeToBroadcastWithMultiplePrimitiveParametersBroadcast(
                        listener, subscriptionQos);
        usleep(1000000);
        testInterfaceProxy->methodToFireBroadcastWithMultiplePrimitiveParameters();
        waitForChange(subscribeBroadcastWithMultiplePrimitiveParametersCallbackDone, 1000);
        ASSERT_TRUE(subscribeBroadcastWithMultiplePrimitiveParametersCallbackDone);
        ASSERT_TRUE(subscribeBroadcastWithMultiplePrimitiveParametersCallbackResult);

        testInterfaceProxy->unsubscribeFromBroadcastWithMultiplePrimitiveParametersBroadcast(
                subscriptionId);
    });
}

class BroadcastWithSingleArrayParameterBroadcastListener
        : public SubscriptionListener<std::vector<std::string>>
{
public:
    BroadcastWithSingleArrayParameterBroadcastListener()
    {
    }

    ~BroadcastWithSingleArrayParameterBroadcastListener()
    {
    }

    void onReceive(const std::vector<std::string>& stringArrayOut)
    {
        JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithSingleArrayParameter - callback - got broadcast");
        if (!IltUtil::checkStringArray(stringArrayOut)) {
            JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                           "callSubscribeBroadcastWithSingleArrayParameter - callback - invalid "
                           "content");
            IltConsumerBroadcastSubscriptionTest::
                    subscribeBroadcastWithSingleArrayParameterCallbackResult = false;
        } else {
            JOYNR_LOG_INFO(
                    iltConsumerBroadcastSubscriptionTestLogger,
                    "callSubscribeBroadcastWithSingleArrayParameter - callback - content OK");
            IltConsumerBroadcastSubscriptionTest::
                    subscribeBroadcastWithSingleArrayParameterCallbackResult = true;
        }
        IltConsumerBroadcastSubscriptionTest::
                subscribeBroadcastWithSingleArrayParameterCallbackDone = true;
    }

    void onError(const joynr::exceptions::JoynrRuntimeException& error)
    {
        JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithSingleArrayParameter - callback - got error");
        IltConsumerBroadcastSubscriptionTest::
                subscribeBroadcastWithSingleArrayParameterCallbackResult = false;
        IltConsumerBroadcastSubscriptionTest::
                subscribeBroadcastWithSingleArrayParameterCallbackDone = true;
    }
};

TEST_F(IltConsumerBroadcastSubscriptionTest, callSubscribeBroadcastWithSingleArrayParameter)
{
    std::string subscriptionId;
    int64_t minInterval_ms = 0;
    int64_t validity = 60000;
    auto subscriptionQos =
            std::make_shared<joynr::OnChangeSubscriptionQos>(validity, minInterval_ms);
    JOYNR_ASSERT_NO_THROW({
        std::shared_ptr<ISubscriptionListener<std::vector<std::string>>> listener(
                new BroadcastWithSingleArrayParameterBroadcastListener());
        subscriptionId = testInterfaceProxy->subscribeToBroadcastWithSingleArrayParameterBroadcast(
                listener, subscriptionQos);
        usleep(1000000);
        testInterfaceProxy->methodToFireBroadcastWithSingleArrayParameter();
        waitForChange(subscribeBroadcastWithSingleArrayParameterCallbackDone, 1000);
        ASSERT_TRUE(subscribeBroadcastWithSingleArrayParameterCallbackDone);
        ASSERT_TRUE(subscribeBroadcastWithSingleArrayParameterCallbackResult);

        testInterfaceProxy->unsubscribeFromBroadcastWithSingleArrayParameterBroadcast(
                subscriptionId);
    });
}

class BroadcastWithMultipleArrayParametersBroadcastListener
        : public SubscriptionListener<
                  std::vector<uint64_t>,
                  std::vector<
                          joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>>
{
public:
    BroadcastWithMultipleArrayParametersBroadcastListener()
    {
    }

    ~BroadcastWithMultipleArrayParametersBroadcastListener()
    {
    }

    void onReceive(const std::vector<uint64_t>& uInt64ArrayOut,
                   const std::vector<
                           joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>&
                           structWithStringArrayArrayOut)
    {
        JOYNR_LOG_INFO(
                iltConsumerBroadcastSubscriptionTestLogger,
                "callSubscribeBroadcastWithMultipleArrayParameters - callback - got broadcast");
        if (!IltUtil::checkUInt64Array(uInt64ArrayOut)) {
            JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                           "callSubscribeBroadcastWithMultipleArrayParameters - callback - invalid "
                           "doubleOut content");
            IltConsumerBroadcastSubscriptionTest::
                    subscribeBroadcastWithMultipleArrayParametersCallbackResult = false;
        } else if (!IltUtil::checkStructWithStringArrayArray(structWithStringArrayArrayOut)) {
            JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                           "callSubscribeBroadcastWithMultipleArrayParameters - callback - invalid "
                           "content");
            IltConsumerBroadcastSubscriptionTest::
                    subscribeBroadcastWithMultipleArrayParametersCallbackResult = false;
        } else {
            JOYNR_LOG_INFO(
                    iltConsumerBroadcastSubscriptionTestLogger,
                    "callSubscribeBroadcastWithMultipleArrayParameters - callback - content OK");
            IltConsumerBroadcastSubscriptionTest::
                    subscribeBroadcastWithMultipleArrayParametersCallbackResult = true;
        }
        IltConsumerBroadcastSubscriptionTest::
                subscribeBroadcastWithMultipleArrayParametersCallbackDone = true;
    }

    void onError(const joynr::exceptions::JoynrRuntimeException& error)
    {
        JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithMultipleArrayParameters - callback - got error");
        IltConsumerBroadcastSubscriptionTest::
                subscribeBroadcastWithMultipleArrayParametersCallbackResult = false;
        IltConsumerBroadcastSubscriptionTest::
                subscribeBroadcastWithMultipleArrayParametersCallbackDone = true;
    }
};

TEST_F(IltConsumerBroadcastSubscriptionTest, callSubscribeBroadcastWithMultipleArrayParameters)
{
    std::string subscriptionId;
    int64_t minInterval_ms = 0;
    int64_t validity = 60000;
    auto subscriptionQos =
            std::make_shared<joynr::OnChangeSubscriptionQos>(validity, minInterval_ms);
    typedef std::shared_ptr<ISubscriptionListener<
            std::vector<uint64_t>,
            std::vector<joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>>>
            listenerType;
    JOYNR_ASSERT_NO_THROW({
        listenerType listener(new BroadcastWithMultipleArrayParametersBroadcastListener());
        subscriptionId =
                testInterfaceProxy->subscribeToBroadcastWithMultipleArrayParametersBroadcast(
                        listener, subscriptionQos);
        usleep(1000000);
        testInterfaceProxy->methodToFireBroadcastWithMultipleArrayParameters();
        waitForChange(subscribeBroadcastWithMultipleArrayParametersCallbackDone, 1000);
        ASSERT_TRUE(subscribeBroadcastWithMultipleArrayParametersCallbackDone);
        ASSERT_TRUE(subscribeBroadcastWithMultipleArrayParametersCallbackResult);

        testInterfaceProxy->unsubscribeFromBroadcastWithMultipleArrayParametersBroadcast(
                subscriptionId);
    });
}

class BroadcastWithSingleEnumerationParameterBroadcastListener
        : public SubscriptionListener<
                  joynr::interlanguagetest::namedTypeCollection2::
                          ExtendedTypeCollectionEnumerationInTypeCollection::Enum>
{
public:
    BroadcastWithSingleEnumerationParameterBroadcastListener()
    {
    }

    ~BroadcastWithSingleEnumerationParameterBroadcastListener()
    {
    }

    void onReceive(const joynr::interlanguagetest::namedTypeCollection2::
                           ExtendedTypeCollectionEnumerationInTypeCollection::Enum& enumerationOut)
    {
        JOYNR_LOG_INFO(
                iltConsumerBroadcastSubscriptionTestLogger,
                "callSubscribeBroadcastWithSingleEnumerationParameter - callback - got broadcast");
        if (enumerationOut != joynr::interlanguagetest::namedTypeCollection2::
                                      ExtendedTypeCollectionEnumerationInTypeCollection::
                                              ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION) {
            JOYNR_LOG_INFO(
                    iltConsumerBroadcastSubscriptionTestLogger,
                    "callSubscribeBroadcastWithSingleEnumerationParameter - callback - invalid "
                    "content");
            IltConsumerBroadcastSubscriptionTest::
                    subscribeBroadcastWithSingleEnumerationParameterCallbackResult = false;
        } else {
            JOYNR_LOG_INFO(
                    iltConsumerBroadcastSubscriptionTestLogger,
                    "callSubscribeBroadcastWithSingleEnumerationParameter - callback - content OK");
            IltConsumerBroadcastSubscriptionTest::
                    subscribeBroadcastWithSingleEnumerationParameterCallbackResult = true;
        }
        IltConsumerBroadcastSubscriptionTest::
                subscribeBroadcastWithSingleEnumerationParameterCallbackDone = true;
    }

    void onError(const joynr::exceptions::JoynrRuntimeException& error)
    {
        JOYNR_LOG_INFO(
                iltConsumerBroadcastSubscriptionTestLogger,
                "callSubscribeBroadcastWithSingleEnumerationParameter - callback - got error");
        IltConsumerBroadcastSubscriptionTest::
                subscribeBroadcastWithSingleEnumerationParameterCallbackResult = false;
        IltConsumerBroadcastSubscriptionTest::
                subscribeBroadcastWithSingleEnumerationParameterCallbackDone = true;
    }
};

TEST_F(IltConsumerBroadcastSubscriptionTest, callSubscribeBroadcastWithSingleEnumerationParameter)
{
    std::string subscriptionId;
    int64_t minInterval_ms = 0;
    int64_t validity = 60000;
    auto subscriptionQos =
            std::make_shared<joynr::OnChangeSubscriptionQos>(validity, minInterval_ms);
    typedef std::shared_ptr<ISubscriptionListener<
            joynr::interlanguagetest::namedTypeCollection2::
                    ExtendedTypeCollectionEnumerationInTypeCollection::Enum>> listenerType;
    JOYNR_ASSERT_NO_THROW({
        listenerType listener(new BroadcastWithSingleEnumerationParameterBroadcastListener());
        subscriptionId =
                testInterfaceProxy->subscribeToBroadcastWithSingleEnumerationParameterBroadcast(
                        listener, subscriptionQos);
        usleep(1000000);
        testInterfaceProxy->methodToFireBroadcastWithSingleEnumerationParameter();
        waitForChange(subscribeBroadcastWithSingleEnumerationParameterCallbackDone, 1000);
        ASSERT_TRUE(subscribeBroadcastWithSingleEnumerationParameterCallbackDone);
        ASSERT_TRUE(subscribeBroadcastWithSingleEnumerationParameterCallbackResult);

        testInterfaceProxy->unsubscribeFromBroadcastWithSingleEnumerationParameterBroadcast(
                subscriptionId);
    });
}

class BroadcastWithMultipleEnumerationParametersBroadcastListener
        : public SubscriptionListener<joynr::interlanguagetest::namedTypeCollection2::
                                              ExtendedEnumerationWithPartlyDefinedValues::Enum,
                                      joynr::interlanguagetest::Enumeration::Enum>
{
public:
    BroadcastWithMultipleEnumerationParametersBroadcastListener()
    {
    }

    ~BroadcastWithMultipleEnumerationParametersBroadcastListener()
    {
    }

    void onReceive(const joynr::interlanguagetest::namedTypeCollection2::
                           ExtendedEnumerationWithPartlyDefinedValues::Enum& extendedEnumerationOut,
                   const joynr::interlanguagetest::Enumeration::Enum& enumerationOut)
    {
        JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithMultipleEnumerationParameters - callback "
                       "- got broadcast");
        if (extendedEnumerationOut !=
            joynr::interlanguagetest::namedTypeCollection2::
                    ExtendedEnumerationWithPartlyDefinedValues::
                            ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES) {
            JOYNR_LOG_INFO(
                    iltConsumerBroadcastSubscriptionTestLogger,
                    "callSubscribeBroadcastWithMultipleEnumerationParameters - callback - invalid "
                    "extendedEnumerationOut content");
            IltConsumerBroadcastSubscriptionTest::
                    subscribeBroadcastWithMultipleEnumerationParametersCallbackResult = false;
        } else if (enumerationOut != joynr::interlanguagetest::Enumeration::ENUM_0_VALUE_1) {
            JOYNR_LOG_INFO(
                    iltConsumerBroadcastSubscriptionTestLogger,
                    "callSubscribeBroadcastWithMultipleEnumerationParameters - callback - invalid "
                    "enumerationOut content");
            IltConsumerBroadcastSubscriptionTest::
                    subscribeBroadcastWithMultipleEnumerationParametersCallbackResult = false;
        } else {
            JOYNR_LOG_INFO(
                    iltConsumerBroadcastSubscriptionTestLogger,
                    "callSubscribeBroadcastWithMultipleEnumerationParameters - callback - content "
                    "OK");
            IltConsumerBroadcastSubscriptionTest::
                    subscribeBroadcastWithMultipleEnumerationParametersCallbackResult = true;
        }
        IltConsumerBroadcastSubscriptionTest::
                subscribeBroadcastWithMultipleEnumerationParametersCallbackDone = true;
    }

    void onError(const joynr::exceptions::JoynrRuntimeException& error)
    {
        JOYNR_LOG_INFO(
                iltConsumerBroadcastSubscriptionTestLogger,
                "callSubscribeBroadcastWithMultipleEnumerationParameters - callback - got error");
        IltConsumerBroadcastSubscriptionTest::
                subscribeBroadcastWithMultipleEnumerationParametersCallbackResult = false;
        IltConsumerBroadcastSubscriptionTest::
                subscribeBroadcastWithMultipleEnumerationParametersCallbackDone = true;
    }
};

TEST_F(IltConsumerBroadcastSubscriptionTest,
       callSubscribeBroadcastWithMultipleEnumerationParameters)
{
    std::string subscriptionId;
    int64_t minInterval_ms = 0;
    int64_t validity = 60000;
    auto subscriptionQos =
            std::make_shared<joynr::OnChangeSubscriptionQos>(validity, minInterval_ms);
    typedef std::shared_ptr<
            ISubscriptionListener<joynr::interlanguagetest::namedTypeCollection2::
                                          ExtendedEnumerationWithPartlyDefinedValues::Enum,
                                  joynr::interlanguagetest::Enumeration::Enum>> listenerType;
    JOYNR_ASSERT_NO_THROW({
        listenerType listener(new BroadcastWithMultipleEnumerationParametersBroadcastListener());
        subscriptionId =
                testInterfaceProxy->subscribeToBroadcastWithMultipleEnumerationParametersBroadcast(
                        listener, subscriptionQos);
        usleep(1000000);
        testInterfaceProxy->methodToFireBroadcastWithMultipleEnumerationParameters();
        waitForChange(subscribeBroadcastWithMultipleEnumerationParametersCallbackDone, 1000);
        ASSERT_TRUE(subscribeBroadcastWithMultipleEnumerationParametersCallbackDone);
        ASSERT_TRUE(subscribeBroadcastWithMultipleEnumerationParametersCallbackResult);

        testInterfaceProxy->unsubscribeFromBroadcastWithMultipleEnumerationParametersBroadcast(
                subscriptionId);
    });
}

class BroadcastWithSingleStructParameterBroadcastListener
        : public SubscriptionListener<
                  joynr::interlanguagetest::namedTypeCollection2::ExtendedStructOfPrimitives>
{
public:
    BroadcastWithSingleStructParameterBroadcastListener()
    {
    }

    ~BroadcastWithSingleStructParameterBroadcastListener()
    {
    }

    void onReceive(const joynr::interlanguagetest::namedTypeCollection2::ExtendedStructOfPrimitives&
                           extendedStructOfPrimitivesOut)
    {
        JOYNR_LOG_INFO(
                iltConsumerBroadcastSubscriptionTestLogger,
                "callSubscribeBroadcastWithSingleStructParameter - callback - got broadcast");
        if (!IltUtil::checkExtendedStructOfPrimitives(extendedStructOfPrimitivesOut)) {
            JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                           "callSubscribeBroadcastWithSingleStructParameter - callback - invalid "
                           "content");
            IltConsumerBroadcastSubscriptionTest::
                    subscribeBroadcastWithSingleStructParameterCallbackResult = false;
        } else {
            JOYNR_LOG_INFO(
                    iltConsumerBroadcastSubscriptionTestLogger,
                    "callSubscribeBroadcastWithSingleStructParameter - callback - content OK");
            IltConsumerBroadcastSubscriptionTest::
                    subscribeBroadcastWithSingleStructParameterCallbackResult = true;
        }
        IltConsumerBroadcastSubscriptionTest::
                subscribeBroadcastWithSingleStructParameterCallbackDone = true;
    }

    void onError(const joynr::exceptions::JoynrRuntimeException& error)
    {
        JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithSingleStructParameter - callback - got error");
        IltConsumerBroadcastSubscriptionTest::
                subscribeBroadcastWithSingleStructParameterCallbackResult = false;
        IltConsumerBroadcastSubscriptionTest::
                subscribeBroadcastWithSingleStructParameterCallbackDone = true;
    }
};

TEST_F(IltConsumerBroadcastSubscriptionTest, callSubscribeBroadcastWithSingleStructParameter)
{
    std::string subscriptionId;
    int64_t minInterval_ms = 0;
    int64_t validity = 60000;
    auto subscriptionQos =
            std::make_shared<joynr::OnChangeSubscriptionQos>(validity, minInterval_ms);
    typedef std::shared_ptr<ISubscriptionListener<
            joynr::interlanguagetest::namedTypeCollection2::ExtendedStructOfPrimitives>>
            listenerType;
    JOYNR_ASSERT_NO_THROW({
        listenerType listener(new BroadcastWithSingleStructParameterBroadcastListener());
        subscriptionId = testInterfaceProxy->subscribeToBroadcastWithSingleStructParameterBroadcast(
                listener, subscriptionQos);
        usleep(1000000);
        testInterfaceProxy->methodToFireBroadcastWithSingleStructParameter();
        waitForChange(subscribeBroadcastWithSingleStructParameterCallbackDone, 1000);
        ASSERT_TRUE(subscribeBroadcastWithSingleStructParameterCallbackDone);
        ASSERT_TRUE(subscribeBroadcastWithSingleStructParameterCallbackResult);

        testInterfaceProxy->unsubscribeFromBroadcastWithSingleStructParameterBroadcast(
                subscriptionId);
    });
}

class BroadcastWithMultipleStructParametersBroadcastListener
        : public SubscriptionListener<
                  joynr::interlanguagetest::namedTypeCollection2::BaseStructWithoutElements,
                  joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedBaseStruct>
{
public:
    BroadcastWithMultipleStructParametersBroadcastListener()
    {
    }

    ~BroadcastWithMultipleStructParametersBroadcastListener()
    {
    }

    void onReceive(const joynr::interlanguagetest::namedTypeCollection2::BaseStructWithoutElements&
                           baseStructWithoutElementsOut,
                   const joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedBaseStruct&
                           extendedExtendedBaseStructOut)
    {
        JOYNR_LOG_INFO(
                iltConsumerBroadcastSubscriptionTestLogger,
                "callSubscribeBroadcastWithMultipleStructParameters - callback - got broadcast");
        if (!IltUtil::checkBaseStructWithoutElements(baseStructWithoutElementsOut)) {
            JOYNR_LOG_INFO(
                    iltConsumerBroadcastSubscriptionTestLogger,
                    "callSubscribeBroadcastWithMultipleStructParameters - callback - invalid "
                    "baseStructWithoutElementsOut content");
            IltConsumerBroadcastSubscriptionTest::
                    subscribeBroadcastWithMultipleStructParametersCallbackResult = false;
        } else if (!IltUtil::checkExtendedExtendedBaseStruct(extendedExtendedBaseStructOut)) {
            JOYNR_LOG_INFO(
                    iltConsumerBroadcastSubscriptionTestLogger,
                    "callSubscribeBroadcastWithMultipleStructParameters - callback - invalid "
                    "extendedExtendedBaseStructOut content");
            IltConsumerBroadcastSubscriptionTest::
                    subscribeBroadcastWithMultipleStructParametersCallbackResult = false;
        } else {
            JOYNR_LOG_INFO(
                    iltConsumerBroadcastSubscriptionTestLogger,
                    "callSubscribeBroadcastWithMultipleStructParameters - callback - content "
                    "OK");
            IltConsumerBroadcastSubscriptionTest::
                    subscribeBroadcastWithMultipleStructParametersCallbackResult = true;
        }
        IltConsumerBroadcastSubscriptionTest::
                subscribeBroadcastWithMultipleStructParametersCallbackDone = true;
    }

    void onError(const joynr::exceptions::JoynrRuntimeException& error)
    {
        JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithMultipleStructParameters - callback - got error");
        IltConsumerBroadcastSubscriptionTest::
                subscribeBroadcastWithMultipleStructParametersCallbackResult = false;
        IltConsumerBroadcastSubscriptionTest::
                subscribeBroadcastWithMultipleStructParametersCallbackDone = true;
    }
};

TEST_F(IltConsumerBroadcastSubscriptionTest, callSubscribeBroadcastWithMultipleStructParameters)
{
    std::string subscriptionId;
    int64_t minInterval_ms = 0;
    int64_t validity = 60000;
    auto subscriptionQos =
            std::make_shared<joynr::OnChangeSubscriptionQos>(validity, minInterval_ms);
    typedef std::shared_ptr<ISubscriptionListener<
            joynr::interlanguagetest::namedTypeCollection2::BaseStructWithoutElements,
            joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedBaseStruct>>
            listenerType;
    JOYNR_ASSERT_NO_THROW({
        listenerType listener(new BroadcastWithMultipleStructParametersBroadcastListener());
        subscriptionId =
                testInterfaceProxy->subscribeToBroadcastWithMultipleStructParametersBroadcast(
                        listener, subscriptionQos);
        usleep(1000000);
        testInterfaceProxy->methodToFireBroadcastWithMultipleStructParameters();
        waitForChange(subscribeBroadcastWithMultipleStructParametersCallbackDone, 1000);
        ASSERT_TRUE(subscribeBroadcastWithMultipleStructParametersCallbackDone);
        ASSERT_TRUE(subscribeBroadcastWithMultipleStructParametersCallbackResult);

        testInterfaceProxy->unsubscribeFromBroadcastWithMultipleStructParametersBroadcast(
                subscriptionId);
    });
}
