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
#ifndef MESSAGESENDER_H
#define MESSAGESENDER_H

#include <memory>
#include <string>
#include <unordered_map>

#include "joynr/IMessageSender.h"
#include "joynr/JoynrExport.h"
#include "joynr/MutableMessageFactory.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"

namespace joynr
{

class IMessageRouter;
class IReplyCaller;
class IDispatcher;
class IKeychain;
class Request;
class Reply;
class MessagingQos;
class SubscriptionRequest;
class BroadcastSubscriptionRequest;
class MulticastSubscriptionRequest;
class SubscriptionReply;
class SubscriptionStop;
class SubscriptionPublication;

/**
  * The class MessageSender enables the exchange of JoynrMessages
  * between the clusterController and libJoynr. It is used by both.
  * It uses a JoynrMessage factory to create a JoynrMessage
  * and sends it via a <Middleware>MessagingStub.
  */

/*
  * JoynrMessageSender needs an Dispatcher, and Dispatcher needs a JoynrMessageSender.
  * This is the case, because the MessageSender needs access to the callerDirectory (via
  * the dispatcher) to store the requestReplyId to the caller). The Dispatcher needs the
  * JoynrMessageSender to send the replies.
  * To break this circle, the JoynrMessageSender is created without Dispatcher* and the Dispatcher
  * is later registered with the JoynrMessageSender. This is a temporary workaround, which can be
  * like this:
  *     Once the JoynrMessage does not have the Reply/RequestId in the header, the dispatcher
  *     will use the participantId to deliver the message to the caller. The caller will then
  *     take the reply/request-ID from the payload and handle it accordingly.
  *     Now the proxy can register the reply-caller with the Dispatcher,
  *     include the reply/requestId into the payload, and pass the payload to the JoynrMessageSender
  *     The MessageSender does not need to register anything with the dispatcher, and thus needs
  *     No reference to the dispatcher.
  */

class JOYNR_EXPORT MessageSender : public IMessageSender
{
public:
    MessageSender(std::shared_ptr<IMessageRouter> messagingRouter,
                  std::shared_ptr<IKeychain> keyChain,
                  std::uint64_t ttlUpliftMs = 0);

    ~MessageSender() override = default;

    void setReplyToAddress(const std::string& _replyToAddress) override;

    /*
      * registers Dispatcher. See above comment why this is necessary.
      */
    void registerDispatcher(std::weak_ptr<IDispatcher> dispatcher) override;

    void sendRequest(const std::string& senderParticipantId,
                     const std::string& receiverParticipantId,
                     const MessagingQos& qos,
                     const Request& request,
                     std::shared_ptr<IReplyCaller> callback,
                     bool isLocalMessage) override;
    /*
     * Prepares and sends a single message
     */
    void sendOneWayRequest(const std::string& senderParticipantId,
                           const std::string& receiverParticipantId,
                           const MessagingQos& qos,
                           const OneWayRequest& request,
                           bool isLocalMessage) override;
    /*
     * Prepares and sends a reply message (an answer to a request)
     */
    void sendReply(const std::string& senderParticipantId,
                   const std::string& receiverParticipantId,
                   const MessagingQos& qos,
                   std::unordered_map<std::string, std::string> prefixedCustomHeaders,
                   const Reply& reply) override;

    void sendSubscriptionRequest(const std::string& senderParticipantId,
                                 const std::string& receiverParticipantId,
                                 const MessagingQos& qos,
                                 const SubscriptionRequest& subscriptionRequest,
                                 bool isLocalMessage) override;

    void sendBroadcastSubscriptionRequest(const std::string& senderParticipantId,
                                          const std::string& receiverParticipantId,
                                          const MessagingQos& qos,
                                          const BroadcastSubscriptionRequest& subscriptionRequest,
                                          bool isLocalMessage) override;

    void sendMulticastSubscriptionRequest(const std::string& senderParticipantId,
                                          const std::string& receiverParticipantId,
                                          const MessagingQos& qos,
                                          const MulticastSubscriptionRequest& subscriptionRequest,
                                          bool isLocalMessage) override;

    void sendSubscriptionReply(const std::string& senderParticipantId,
                               const std::string& receiverParticipantId,
                               const MessagingQos& qos,
                               const SubscriptionReply& subscriptionReply) override;

    void sendSubscriptionStop(const std::string& senderParticipantId,
                              const std::string& receiverParticipantId,
                              const MessagingQos& qos,
                              const SubscriptionStop& subscriptionStop) override;

    void sendSubscriptionPublication(const std::string& senderParticipantId,
                                     const std::string& receiverParticipantId,
                                     const MessagingQos& qos,
                                     SubscriptionPublication&& subscriptionPublication) override;

    void sendMulticast(const std::string& fromParticipantId,
                       const MulticastPublication& multicastPublication,
                       const MessagingQos& messagingQos) override;

private:
    DISALLOW_COPY_AND_ASSIGN(MessageSender);
    std::weak_ptr<IDispatcher> _dispatcher;
    std::shared_ptr<IMessageRouter> _messageRouter;
    MutableMessageFactory _messageFactory;
    std::string _replyToAddress;
    ADD_LOGGER(MessageSender)
};

} // namespace joynr
#endif // MESSAGESENDER_H
