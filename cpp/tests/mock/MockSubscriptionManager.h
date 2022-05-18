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
#ifndef TESTS_MOCK_MOCKSUBSCRIPTIONMANAGER_H
#define TESTS_MOCK_MOCKSUBSCRIPTIONMANAGER_H

#include "tests/utils/Gmock.h"

#include "joynr/SubscriptionManager.h"
#include "joynr/MulticastSubscriptionRequest.h"
#include "joynr/SubscriptionRequest.h"

class MockSubscriptionManager : public joynr::SubscriptionManager {
public:
    using SubscriptionManager::SubscriptionManager;

    MOCK_METHOD1(getSubscriptionCallback,std::shared_ptr<joynr::ISubscriptionCallback>(const std::string& subscriptionId));
    MOCK_METHOD5(registerSubscription,void(const std::string& subscribeToName,
                                           std::shared_ptr<joynr::ISubscriptionCallback> subscriptionCaller,
                                           std::shared_ptr<joynr::ISubscriptionListenerBase> subscriptionListener,
                                           std::shared_ptr<joynr::SubscriptionQos> qos,
                                           joynr::SubscriptionRequest& subscriptionRequest));
    MOCK_METHOD10(registerSubscription,void(const std::string& subscribeToName,
                                           const std::string& subscriberParticipantId,
                                           const std::string& providerParticipantId,
                                           const std::vector<std::string>& partitions,
                                           std::shared_ptr<joynr::ISubscriptionCallback> subscriptionCaller,
                                           std::shared_ptr<joynr::ISubscriptionListenerBase> subscriptionListener,
                                           std::shared_ptr<joynr::SubscriptionQos> qos,
                                           joynr::MulticastSubscriptionRequest& subscriptionRequest,
                                           std::function<void()> onSuccess,
                                           std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError));
    MOCK_METHOD1(unregisterSubscription, void(const std::string& subscriptionId));
    MOCK_METHOD1(touchSubscriptionState,void(const std::string& subscriptionId));
    MOCK_METHOD1(
        getMulticastSubscriptionCallback,
        std::shared_ptr<joynr::ISubscriptionCallback>(const std::string& multicastId)
    );
    MOCK_METHOD1(
        getSubscriptionListener,
        std::shared_ptr<joynr::ISubscriptionListenerBase>(
                const std::string& subscriptionId
        )
    );
    MOCK_METHOD1(
        getMulticastSubscriptionListeners,
        std::forward_list<std::shared_ptr<joynr::ISubscriptionListenerBase>>(
                const std::string& multicastId
        )
    );
};

#endif // TESTS_MOCK_MOCKSUBSCRIPTIONMANAGER_H
