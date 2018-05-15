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
#include "AbstractRobustnessTest.h"
#include "joynr/MulticastSubscriptionQos.h"
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"

using joynr::MulticastSubscriptionQos;
using joynr::OnChangeWithKeepAliveSubscriptionQos;

class RobustnessTestProviderCrash : public AbstractRobustnessTest
{
};

TEST_F(RobustnessTestProviderCrash, call_methodWithStringParameters)
{
    callMethodWithStringParameters();
}

TEST_F(RobustnessTestProviderCrash, call_methodWithStringParametersAfterProviderRestart)
{
    killProvider();
    startProvider();
    callMethodWithStringParameters();
}

// this test will temporarily be skipped as it currently does not work, will be enabled later on
TEST_F(RobustnessTestProviderCrash, DISABLED_call_methodWithStringParametersBeforeProviderRestart)
{
    // wait for the provider from the last test case to be expired
    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    // kill the provider before the request is sent
    callMethodWithStringParametersBeforeCcOrProviderRestart(false, true);
}

TEST_F(RobustnessTestProviderCrash, subscribeTo_broadcastWithSingleStringParameter)
{
    Semaphore subscriptionRegisteredSemaphore(0);
    Semaphore publicationSemaphore(0);
    std::string subscriptionId;
    auto mockListener = std::make_shared<MockSubscriptionListenerOneType<std::string>>();

    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener, onReceive(_))
            .WillRepeatedly(ReleaseSemaphore(&publicationSemaphore));
    EXPECT_CALL(*mockListener, onSubscribed(_)).WillRepeatedly(
            DoAll(SaveArg<0>(&subscriptionId), ReleaseSemaphore(&subscriptionRegisteredSemaphore)));

    auto subscriptionQos = std::make_shared<MulticastSubscriptionQos>();
    subscriptionQos->setValidityMs(600000);
    proxy->subscribeToBroadcastWithSingleStringParameterBroadcast(mockListener, subscriptionQos);
    ASSERT_TRUE(subscriptionRegisteredSemaphore.waitFor(std::chrono::seconds(10)));

    // wait for the provider from the last test case to be expired
    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    killProvider();
    startProvider();

    try {
        proxy->startFireBroadcastWithSingleStringParameter();
        // Wait for a subscription message to arrive
        ASSERT_TRUE(publicationSemaphore.waitFor(std::chrono::seconds(60)))
                << "broadcastWithSingleStringParameter did not arrive in time";
    } catch (const joynr::exceptions::JoynrRuntimeException& e) {
        ADD_FAILURE() << "startFireBroadcastWithSingleStringParameter failed with: "
                      << e.getMessage();
    }
    try {
        proxy->stopFireBroadcastWithSingleStringParameter();
    } catch (const joynr::exceptions::JoynrRuntimeException& e) {
        ADD_FAILURE() << "stopFireBroadcastWithSingleStringParameter failed with: "
                      << e.getMessage();
    }
    proxy->unsubscribeFromBroadcastWithSingleStringParameterBroadcast(subscriptionId);
}

TEST_F(RobustnessTestProviderCrash, subscribeToAttributeString)
{
    Semaphore subscriptionRegisteredSemaphore(0);
    Semaphore publicationSemaphore(0);
    std::string subscriptionId;
    auto mockListener = std::make_shared<MockSubscriptionListenerOneType<std::string>>();

    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener, onReceive(_))
            .WillRepeatedly(ReleaseSemaphore(&publicationSemaphore));
    EXPECT_CALL(*mockListener, onSubscribed(_)).WillRepeatedly(
            DoAll(SaveArg<0>(&subscriptionId), ReleaseSemaphore(&subscriptionRegisteredSemaphore)));

    auto subscriptionQos = std::make_shared<OnChangeWithKeepAliveSubscriptionQos>();
    subscriptionQos->setMinIntervalMs(5 * 1000);
    subscriptionQos->setMaxIntervalMs(30 * 1000);
    subscriptionQos->setValidityMs(120 * 1000);
    proxy->subscribeToAttributeString(mockListener, subscriptionQos);
    ASSERT_TRUE(subscriptionRegisteredSemaphore.waitFor(std::chrono::seconds(10)));

    // the first publication should arrive immediately after subscription is done
    try {
        // Wait for a subscription message to arrive
        ASSERT_TRUE(publicationSemaphore.waitFor(std::chrono::seconds(60)))
                << "attribute publication did not arrive in time";
    } catch (const joynr::exceptions::JoynrRuntimeException& e) {
        ADD_FAILURE() << "waiting for attribute publication failed with: " << e.getMessage();
    }

    // kill and restart the provider while the time period until the next
    // publication happens is passing; the time period must be long enough
    // so that no further publication is sent until the provider got killed
    killProvider();
    startProvider();

    try {
        // Wait for a subscription message to arrive
        ASSERT_TRUE(publicationSemaphore.waitFor(std::chrono::seconds(60)))
                << "attribute publication did not arrive in time";
    } catch (const joynr::exceptions::JoynrRuntimeException& e) {
        ADD_FAILURE() << "waiting for attribute publication failed with: " << e.getMessage();
    }
    proxy->unsubscribeFromAttributeString(subscriptionId);
}
