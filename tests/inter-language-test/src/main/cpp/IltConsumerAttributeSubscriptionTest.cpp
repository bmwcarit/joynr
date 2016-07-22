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
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"

using namespace ::testing;

class IltConsumerAttributeSubscriptionTest : public IltAbstractConsumerTest
{
public:
    IltConsumerAttributeSubscriptionTest() = default;

    static volatile bool subscribeAttributeEnumerationCallbackDone;
    static volatile bool subscribeAttributeEnumerationCallbackResult;
    static volatile bool subscribeAttributeWithExceptionFromGetterCallbackDone;
    static volatile bool subscribeAttributeWithExceptionFromGetterCallbackResult;
};

joynr::Logger iltConsumerAttributeSubscriptionTestLogger("IltConsumerAttributeSubscriptionTest");

// SUBSCRIBE ATTRIBUTE ENUMERATION

volatile bool IltConsumerAttributeSubscriptionTest::subscribeAttributeEnumerationCallbackDone =
        false;
volatile bool IltConsumerAttributeSubscriptionTest::subscribeAttributeEnumerationCallbackResult =
        false;
volatile bool IltConsumerAttributeSubscriptionTest::
        subscribeAttributeWithExceptionFromGetterCallbackDone = false;
volatile bool IltConsumerAttributeSubscriptionTest::
        subscribeAttributeWithExceptionFromGetterCallbackResult = false;

class AttributeEnumerationListener
        : public SubscriptionListener<joynr::interlanguagetest::Enumeration::Enum>
{
public:
    AttributeEnumerationListener() = default;

    ~AttributeEnumerationListener() override = default;

    void onReceive(const joynr::interlanguagetest::Enumeration::Enum& enumerationOut) override
    {
        JOYNR_LOG_INFO(iltConsumerAttributeSubscriptionTestLogger,
                       "callSubscribeAttributeEnumeration - callback - got broadcast");
        if (enumerationOut != joynr::interlanguagetest::Enumeration::ENUM_0_VALUE_2) {
            JOYNR_LOG_INFO(iltConsumerAttributeSubscriptionTestLogger,
                           "callSubscribeAttributeEnumeration - callback - invalid "
                           "content");
            IltConsumerAttributeSubscriptionTest::subscribeAttributeEnumerationCallbackResult =
                    false;
        } else {
            JOYNR_LOG_INFO(iltConsumerAttributeSubscriptionTestLogger,
                           "callSubscribeAttributeEnumeration - callback - content OK");
            IltConsumerAttributeSubscriptionTest::subscribeAttributeEnumerationCallbackResult =
                    true;
        }
        IltConsumerAttributeSubscriptionTest::subscribeAttributeEnumerationCallbackDone = true;
    }

    void onError(const joynr::exceptions::JoynrRuntimeException& error) override
    {
        JOYNR_LOG_INFO(iltConsumerAttributeSubscriptionTestLogger,
                       "callSubscribeAttributeEnumeration - callback - got error");
        IltConsumerAttributeSubscriptionTest::subscribeAttributeEnumerationCallbackResult = false;
        IltConsumerAttributeSubscriptionTest::subscribeAttributeEnumerationCallbackDone = true;
    }
};

TEST_F(IltConsumerAttributeSubscriptionTest, callSubscribeAttributeEnumeration)
{
    std::string subscriptionId;
    int64_t minInterval_ms = 0;
    int64_t validity = 60000;
    joynr::OnChangeSubscriptionQos subscriptionQos(validity, minInterval_ms);
    JOYNR_ASSERT_NO_THROW({
        joynr::interlanguagetest::Enumeration::Enum enumerationArg =
                joynr::interlanguagetest::Enumeration::ENUM_0_VALUE_2;
        testInterfaceProxy->setAttributeEnumeration(enumerationArg);

        std::shared_ptr<ISubscriptionListener<joynr::interlanguagetest::Enumeration::Enum>>
                listener(new AttributeEnumerationListener());
        subscriptionId =
                testInterfaceProxy->subscribeToAttributeEnumeration(listener, subscriptionQos);
        usleep(1000000);
        waitForChange(subscribeAttributeEnumerationCallbackDone, 1000);
        ASSERT_TRUE(subscribeAttributeEnumerationCallbackDone);
        ASSERT_TRUE(subscribeAttributeEnumerationCallbackResult);

        testInterfaceProxy->unsubscribeFromAttributeEnumeration(subscriptionId);
    });
}

class AttributeWithExceptionFromGetterListener : public SubscriptionListener<bool>
{
public:
    AttributeWithExceptionFromGetterListener() = default;

    ~AttributeWithExceptionFromGetterListener() override = default;

    void onReceive(const bool& value) override
    {
        JOYNR_LOG_INFO(iltConsumerAttributeSubscriptionTestLogger,
                       "callSubscribeAttributeWithExceptionFromGetter - callback - "
                       "unexpectedly got broadcast");
        IltConsumerAttributeSubscriptionTest::
                subscribeAttributeWithExceptionFromGetterCallbackResult = false;
        IltConsumerAttributeSubscriptionTest::
                subscribeAttributeWithExceptionFromGetterCallbackDone = true;
    }

    void onError(const joynr::exceptions::JoynrRuntimeException& error) override
    {
        if (error.getTypeName() == "joynr.exceptions.ProviderRuntimeException") {
            if (error.getMessage() == "Exception from getAttributeWithExceptionFromGetter") {
                JOYNR_LOG_INFO(
                        iltConsumerAttributeSubscriptionTestLogger,
                        "callSubscribeAttributeWithExceptionFromGetter - callback - got expected "
                        "exception");
                IltConsumerAttributeSubscriptionTest::
                        subscribeAttributeWithExceptionFromGetterCallbackResult = true;
            } else {
                JOYNR_LOG_INFO(iltConsumerAttributeSubscriptionTestLogger,
                               "callSubscribeAttributeWithExceptionFromGetter - callback - got "
                               "ProviderRuntimeException with wrong message");
                JOYNR_LOG_INFO(iltConsumerAttributeSubscriptionTestLogger, error.getMessage());
                IltConsumerAttributeSubscriptionTest::
                        subscribeAttributeWithExceptionFromGetterCallbackResult = false;
            }
        } else {
            JOYNR_LOG_INFO(iltConsumerAttributeSubscriptionTestLogger,
                           "callSubscribeAttributeWithExceptionFromGetter - callback - got invalid "
                           "exception "
                           "type");
            JOYNR_LOG_INFO(iltConsumerAttributeSubscriptionTestLogger, error.getTypeName());
            JOYNR_LOG_INFO(iltConsumerAttributeSubscriptionTestLogger, error.getMessage());
            IltConsumerAttributeSubscriptionTest::
                    subscribeAttributeWithExceptionFromGetterCallbackResult = false;
        }
        IltConsumerAttributeSubscriptionTest::
                subscribeAttributeWithExceptionFromGetterCallbackDone = true;
    }
};

TEST_F(IltConsumerAttributeSubscriptionTest, callSubscribeAttributeWithExceptionFromGetter)
{
    std::string subscriptionId;
    int64_t minInterval_ms = 0;
    int64_t validity = 60000;
    joynr::OnChangeSubscriptionQos subscriptionQos(validity, minInterval_ms);
    JOYNR_ASSERT_NO_THROW({
        std::shared_ptr<ISubscriptionListener<bool>> listener(
                new AttributeWithExceptionFromGetterListener());
        subscriptionId = testInterfaceProxy->subscribeToAttributeWithExceptionFromGetter(
                listener, subscriptionQos);
        // Waiting one second
        waitForChange(subscribeAttributeWithExceptionFromGetterCallbackDone, 1000);
        ASSERT_TRUE(subscribeAttributeWithExceptionFromGetterCallbackDone);
        ASSERT_TRUE(subscribeAttributeWithExceptionFromGetterCallbackResult);

        testInterfaceProxy->unsubscribeFromAttributeWithExceptionFromGetter(subscriptionId);
    });
}
