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
#ifndef IMESSAGESENDER_H
#define IMESSAGESENDER_H

#include <memory>
#include <string>
#include <unordered_map>

#include "joynr/IPublicationSender.h"

namespace joynr
{

class IReplyCaller;
class IDispatcher;
class Request;
class OneWayRequest;
class Reply;
class MessagingQos;
class SubscriptionRequest;
class BroadcastSubscriptionRequest;
class MulticastSubscriptionRequest;
class MulticastPublication;
class SubscriptionStop;

/**
 * The interface JoynrMessageSender enables the exchange of JoynrMessages
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

class IMessageSender : public IPublicationSender
{
public:
    ~IMessageSender() override = default;

    virtual void setReplyToAddress(const std::string& replyAddress) = 0;

    /*
     * registers Dispatcher. See above comment why this is necessary.
     */
    virtual void registerDispatcher(std::weak_ptr<IDispatcher> dispatcher) = 0;

    /*
     * Prepares and sends a request message (such as issued by a Proxy)
     */
    virtual void sendRequest(const std::string& senderParticipantId,
                             const std::string& receiverParticipantId,
                             const MessagingQos& qos,
                             const Request& request,
                             std::shared_ptr<IReplyCaller> callback,
                             bool isLocalMessage) = 0;

    /*
     * Prepares and sends a single message
     */
    virtual void sendOneWayRequest(const std::string& senderParticipantId,
                                   const std::string& receiverParticipantId,
                                   const MessagingQos& qos,
                                   const OneWayRequest& request,
                                   bool isLocalMessage) = 0;

    /*
     * Prepares and sends a reply message (an answer to a request)
     */
    virtual void sendReply(const std::string& senderParticipantId,
                           const std::string& receiverParticipantId,
                           const MessagingQos& qos,
                           std::unordered_map<std::string, std::string> prefixedCustomHeaders,
                           const Reply& reply) = 0;

    virtual void sendSubscriptionRequest(const std::string& senderParticipantId,
                                         const std::string& receiverParticipantId,
                                         const MessagingQos& qos,
                                         const SubscriptionRequest& subscriptionRequest,
                                         bool isLocalMessage) = 0;

    virtual void sendBroadcastSubscriptionRequest(
            const std::string& senderParticipantId,
            const std::string& receiverParticipantId,
            const MessagingQos& qos,
            const BroadcastSubscriptionRequest& subscriptionRequest,
            bool isLocalMessage) = 0;

    virtual void sendSubscriptionStop(const std::string& senderParticipantId,
                                      const std::string& receiverParticipantId,
                                      const MessagingQos& qos,
                                      const SubscriptionStop& subscriptionStop) = 0;

    virtual void sendMulticast(const std::string& fromParticipantId,
                               const MulticastPublication& multicastPublication,
                               const MessagingQos& messagingQos) = 0;

    virtual void removeRoutingEntry(const std::string& sparticipantId) = 0;
};

} // namespace joynr
#endif // IMESSAGESENDER_H
