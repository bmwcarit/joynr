/*
 * #%L
 * %%
 * Copyright (C) 2016 - 2016 BMW Car IT GmbH
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
#include "joynr/OnChangeSubscriptionQos.h"

using joynr::OnChangeSubscriptionQos;

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

TEST_F(RobustnessTestProviderCrash, call_methodWithStringParametersBeforeProviderRestart)
{
    // kill the provider before the request is sent
    callMethodWithStringParametersBeforeCcOrProviderRestart(false, true);
}

template <typename T>
class MockSubscriptionListenerOneType : public joynr::ISubscriptionListener<T>
{
public:
    MOCK_METHOD1_T(onReceive, void(const T& value));
    MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrRuntimeException&));
};

TEST_F(RobustnessTestProviderCrash, subscribeTo_broadcastWithSingleStringParameter)
{
    Semaphore semaphore(0);
    auto mockListener = std::make_shared<MockSubscriptionListenerOneType<std::string>>();

    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockListener, onReceive(_)).WillRepeatedly(ReleaseSemaphore(&semaphore));

    OnChangeSubscriptionQos subscriptionQos;
    subscriptionQos.setValidityMs(600000);
    proxy->subscribeToBroadcastWithSingleStringParameterBroadcast(mockListener, subscriptionQos);
    // This wait is necessary, because subcriptions are async, and a broadcast could occur
    // before the subscription has started.
    std::this_thread::sleep_for(std::chrono::seconds(3));

    killProvider();
    startProvider();

    try {
        proxy->startFireBroadcastWithSingleStringParameter();
        // Wait for a subscription message to arrive
        ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(60)))
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
    // TOOD similar test for subscriptions after provider restart and before provider restart
}
