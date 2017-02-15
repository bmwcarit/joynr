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

#include <chrono>

#include "joynr/MulticastSubscriptionQos.h"

class RobustnessTestCcCrash : public AbstractRobustnessTest
{
public:
    RobustnessTestCcCrash() = default;
};

TEST_F(RobustnessTestCcCrash, call_methodWithStringParameters)
{
    callMethodWithStringParameters();
}

TEST_F(RobustnessTestCcCrash, call_methodWithStringParametersAfterCCRestart)
{
    callMethodWithStringParametersAfterCcRestart();
}

TEST_F(RobustnessTestCcCrash, call_methodWithStringParametersBeforeCCRestart)
{
    // kill the cluster controller before request is sent
    callMethodWithStringParametersBeforeCcOrProviderRestart(true, false);
}

TEST_F(RobustnessTestCcCrash, subscribeToMulticast_WithCCCrash)
{
    auto qos = std::make_shared<joynr::MulticastSubscriptionQos>();
    auto subscriptionListener = std::make_shared<MockSubscriptionListenerOneType<std::string>>();

    Semaphore subscriptionRegisteredSemaphore(0);
    Semaphore publicationSemaphore(0);

    EXPECT_CALL(*subscriptionListener, onSubscribed(_)).Times(1).WillOnce(
            ReleaseSemaphore(&subscriptionRegisteredSemaphore));

    EXPECT_CALL(*subscriptionListener, onReceive(_)).Times(1).WillOnce(
            ReleaseSemaphore(&publicationSemaphore));

    proxy->subscribeToBroadcastWithSingleStringParameterBroadcast(subscriptionListener, qos);

    EXPECT_TRUE(subscriptionRegisteredSemaphore.waitFor(std::chrono::seconds(2)));

    killClusterController();
    startClusterController();

    proxy->methodToFireBroadcastWithSingleStringParameter();

    EXPECT_TRUE(publicationSemaphore.waitFor(std::chrono::seconds(2)));
}
