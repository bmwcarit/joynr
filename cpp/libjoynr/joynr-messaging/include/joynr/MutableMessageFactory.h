/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
#ifndef MUTABLEMESSAGEFACTORY_H
#define MUTABLEMESSAGEFACTORY_H

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include "joynr/JoynrExport.h"
#include "joynr/Logger.h"
#include "joynr/MutableMessage.h"
#include "joynr/PrivateCopyAssign.h"

namespace joynr
{

class IKeychain;
class MessagingQos;
class MulticastPublication;
class OneWayRequest;
class Request;
class Reply;
class SubscriptionPublication;
class SubscriptionStop;
class SubscriptionReply;
class SubscriptionRequest;
class BroadcastSubscriptionRequest;
class MulticastSubscriptionRequest;
class IPlatformSecurityManager;

/**
  * The MutableMessageFactory creates MutableMessages. It sets the headers and
  * payload according to the message type. It is used by the MessageSender.
  */
class JOYNR_EXPORT MutableMessageFactory
{
public:
    explicit MutableMessageFactory(std::uint64_t ttlUpliftMs = 0,
                                   std::shared_ptr<IKeychain> keyChain = nullptr);
    ~MutableMessageFactory();

    MutableMessage createRequest(const std::string& senderId,
                                 const std::string& receiverId,
                                 const MessagingQos& qos,
                                 const Request& payload,
                                 bool isLocalMessage) const;

    MutableMessage createReply(const std::string& senderId,
                               const std::string& receiverId,
                               const MessagingQos& qos,
                               std::unordered_map<std::string, std::string>&& prefixedCustomHeaders,
                               const Reply& payload) const;

    MutableMessage createOneWayRequest(const std::string& senderId,
                                       const std::string& receiverId,
                                       const MessagingQos& qos,
                                       const OneWayRequest& payload,
                                       bool isLocalMessage) const;

    MutableMessage createSubscriptionPublication(const std::string& senderId,
                                                 const std::string& receiverId,
                                                 const MessagingQos& qos,
                                                 const SubscriptionPublication& payload) const;

    MutableMessage createSubscriptionRequest(const std::string& senderId,
                                             const std::string& receiverId,
                                             const MessagingQos& qos,
                                             const SubscriptionRequest& payload,
                                             bool isLocalMessage) const;

    MutableMessage createMulticastSubscriptionRequest(const std::string& senderId,
                                                      const std::string& receiverId,
                                                      const MessagingQos& qos,
                                                      const MulticastSubscriptionRequest& payload,
                                                      bool isLocalMessage) const;

    MutableMessage createBroadcastSubscriptionRequest(const std::string& senderId,
                                                      const std::string& receiverId,
                                                      const MessagingQos& qos,
                                                      const BroadcastSubscriptionRequest& payload,
                                                      bool isLocalMessage) const;

    MutableMessage createSubscriptionReply(const std::string& senderId,
                                           const std::string& receiverId,
                                           const MessagingQos& qos,
                                           const SubscriptionReply& payload) const;

    MutableMessage createSubscriptionStop(const std::string& senderId,
                                          const std::string& receiverId,
                                          const MessagingQos& qos,
                                          const SubscriptionStop& payload) const;

    MutableMessage createMulticastPublication(const std::string& senderId,
                                              const MessagingQos& qos,
                                              const MulticastPublication& payload) const;

private:
    DISALLOW_COPY_AND_ASSIGN(MutableMessageFactory);

    void initMsg(MutableMessage& msg,
                 const std::string& senderParticipantId,
                 const std::string& receiverParticipantId,
                 const MessagingQos& qos,
                 std::string&& payload,
                 bool upliftTtl = true) const;

    std::unique_ptr<IPlatformSecurityManager> _securityManager;
    std::uint64_t _ttlUpliftMs;
    std::shared_ptr<IKeychain> _keyChain;
    ADD_LOGGER(MutableMessageFactory)
};

} // namespace joynr
#endif // MUTABLEMESSAGEFACTORY_H
