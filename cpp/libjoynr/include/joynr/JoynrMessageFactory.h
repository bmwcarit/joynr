/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
#ifndef JOYNRMESSAGEFACTORY_H
#define JOYNRMESSAGEFACTORY_H

#include <string>

#include "joynr/JoynrMessage.h"
#include "joynr/PrivateCopyAssign.h"

#include "joynr/JoynrExport.h"
#include "joynr/Logger.h"
#include "joynr/IPlatformSecurityManager.h"
#include "joynr/OneWayRequest.h"
namespace joynr
{

class MessagingQos;
class Request;
class Reply;
class SubscriptionPublication;
class SubscriptionStop;
class SubscriptionReply;
class SubscriptionRequest;
class BroadcastSubscriptionRequest;

/**
  * The JoynrMessageFactory creates JoynrMessages. It sets the headers and
  * payload according to the message type. It is used by the JoynrMessageSender.
  */
class JOYNR_EXPORT JoynrMessageFactory
{
public:
    JoynrMessageFactory();

    JoynrMessage createRequest(const std::string& senderId,
                               const std::string& receiverId,
                               const MessagingQos& qos,
                               const Request& payload) const;

    JoynrMessage createReply(const std::string& senderId,
                             const std::string& receiverId,
                             const MessagingQos& qos,
                             const Reply& payload) const;

    JoynrMessage createOneWayRequest(const std::string& senderId,
                                     const std::string& receiverId,
                                     const MessagingQos& qos,
                                     const OneWayRequest& payload) const;

    JoynrMessage createSubscriptionPublication(const std::string& senderId,
                                               const std::string& receiverId,
                                               const MessagingQos& qos,
                                               const SubscriptionPublication& payload) const;

    JoynrMessage createSubscriptionRequest(const std::string& senderId,
                                           const std::string& receiverId,
                                           const MessagingQos& qos,
                                           const SubscriptionRequest& payload) const;

    JoynrMessage createBroadcastSubscriptionRequest(
            const std::string& senderId,
            const std::string& receiverId,
            const MessagingQos& qos,
            const BroadcastSubscriptionRequest& payload) const;

    JoynrMessage createSubscriptionReply(const std::string& senderId,
                                         const std::string& receiverId,
                                         const MessagingQos& qos,
                                         const SubscriptionReply& payload) const;

    JoynrMessage createSubscriptionStop(const std::string& senderId,
                                        const std::string& receiverId,
                                        const MessagingQos& qos,
                                        const SubscriptionStop& payload) const;

private:
    DISALLOW_COPY_AND_ASSIGN(JoynrMessageFactory);

    void initMsg(JoynrMessage& msg,
                 const std::string& senderParticipantId,
                 const std::string& receiverParticipantId,
                 const std::int64_t ttl,
                 const std::string& payload) const;

    void initReplyMsg(JoynrMessage& msg,
                      const std::string& senderParticipantId,
                      const std::string& receiverParticipantId,
                      const std::int64_t ttl,
                      const Reply& payload) const;

    void initSubscriptionPublicationMsg(JoynrMessage& msg,
                                        const std::string& senderParticipantId,
                                        const std::string& receiverParticipantId,
                                        const std::int64_t ttl,
                                        const SubscriptionPublication& payload) const;

    std::unique_ptr<IPlatformSecurityManager> securityManager;
    ADD_LOGGER(JoynrMessageFactory);
};

} // namespace joynr
#endif // JOYNRMESSAGEFACTORY_H
