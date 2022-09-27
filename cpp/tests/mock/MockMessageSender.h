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
#ifndef TESTS_MOCK_MOCKMESSAGESENDER_H
#define TESTS_MOCK_MOCKMESSAGESENDER_H

#include <memory>
#include <string>

#include "tests/utils/Gmock.h"

#include "joynr/BroadcastSubscriptionRequest.h"
#include "joynr/IDispatcher.h"
#include "joynr/IMessageSender.h"
#include "joynr/IReplyCaller.h"
#include "joynr/MessagingQos.h"
#include "joynr/MulticastPublication.h"
#include "joynr/MulticastSubscriptionRequest.h"
#include "joynr/OneWayRequest.h"
#include "joynr/Reply.h"
#include "joynr/Request.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/SubscriptionReply.h"
#include "joynr/SubscriptionRequest.h"
#include "joynr/SubscriptionStop.h"

class MockMessageSender : public joynr::IMessageSender
{
public:
    MOCK_METHOD1(setReplyToAddress, void(const std::string& replyToAddress));

    MOCK_METHOD1(registerDispatcher, void(std::weak_ptr<joynr::IDispatcher> dispatcher));

    MOCK_METHOD6(sendRequest,
                 void(const std::string& senderParticipantId,
                      const std::string& receiverParticipantId,
                      const joynr::MessagingQos& qos,
                      const joynr::Request& request,
                      std::shared_ptr<joynr::IReplyCaller> callback,
                      bool isLocalMessage));

    MOCK_METHOD5(sendOneWayRequest,
                 void(const std::string& senderParticipantId,
                      const std::string& receiverParticipantId,
                      const joynr::MessagingQos& qos,
                      const joynr::OneWayRequest& request,
                      bool isLocalMessage));

    MOCK_METHOD5(sendReply,
                 void(const std::string& senderParticipantId,
                      const std::string& receiverParticipantId,
                      const joynr::MessagingQos& qos,
                      std::unordered_map<std::string, std::string> prefixedCustomHeaders,
                      const joynr::Reply& reply));

    MOCK_METHOD5(sendSubscriptionRequest,
                 void(const std::string& senderParticipantId,
                      const std::string& receiverParticipantId,
                      const joynr::MessagingQos& qos,
                      const joynr::SubscriptionRequest& subscriptionRequest,
                      bool isLocalMessage));

    MOCK_METHOD5(sendBroadcastSubscriptionRequest,
                 void(const std::string& senderParticipantId,
                      const std::string& receiverParticipantId,
                      const joynr::MessagingQos& qos,
                      const joynr::BroadcastSubscriptionRequest& subscriptionRequest,
                      bool isLocalMessage));

    MOCK_METHOD4(sendSubscriptionReply,
                 void(const std::string& senderParticipantId,
                      const std::string& receiverParticipantId,
                      const joynr::MessagingQos& qos,
                      const joynr::SubscriptionReply& subscriptionReply));

    MOCK_METHOD4(sendSubscriptionStop,
                 void(const std::string& senderParticipantId,
                      const std::string& receiverParticipantId,
                      const joynr::MessagingQos& qos,
                      const joynr::SubscriptionStop& subscriptionStop));

    MOCK_METHOD4(sendSubscriptionPublicationMock,
                 void(const std::string& senderParticipantId,
                      const std::string& receiverParticipantId,
                      const joynr::MessagingQos& qos,
                      const joynr::SubscriptionPublication& subscriptionPublication));

    MOCK_METHOD3(sendMulticast,
                 void(const std::string& fromParticipantId,
                      const joynr::MulticastPublication& multicastPublication,
                      const joynr::MessagingQos& messagingQos));

    MOCK_METHOD5(sendMulticastSubscriptionRequest,
                 void(const std::string& senderParticipantId,
                      const std::string& receiverParticipantId,
                      const joynr::MessagingQos& qos,
                      const joynr::MulticastSubscriptionRequest& subscriptionRequest,
                      bool isLocalMessage));

    MOCK_METHOD1(removeRoutingEntry, void(const std::string& participantId));

    void sendSubscriptionPublication(const std::string& senderParticipantId,
                                     const std::string& receiverParticipantId,
                                     const joynr::MessagingQos& qos,
                                     joynr::SubscriptionPublication&& subscriptionPublication)
    {
        sendSubscriptionPublicationMock(
                senderParticipantId, receiverParticipantId, qos, subscriptionPublication);
    }
};

#endif // TESTS_MOCK_MOCKMESSAGESENDER_H
