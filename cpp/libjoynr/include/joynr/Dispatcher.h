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
#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <memory>
#include <string>

#include "joynr/IDispatcher.h"
#include "joynr/JoynrExport.h"
#include "joynr/LibJoynrDirectories.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/ThreadPool.h"

namespace boost
{
namespace asio
{
class io_service;
} // namespace asio
} // namespace boost

namespace joynr
{

class ImmutableMessage;
class IReplyCaller;
class MessagingQos;
class RequestCaller;
class IMessageSender;

class JOYNR_EXPORT Dispatcher : public IDispatcher
{

public:
    Dispatcher(std::shared_ptr<IMessageSender> messageSender,
               boost::asio::io_service& ioService,
               int maxThreads = 1);

    ~Dispatcher() override;

    void addReplyCaller(const std::string& requestReplyId,
                        std::shared_ptr<IReplyCaller> replyCaller,
                        const MessagingQos& qosSettings) override;

    void removeReplyCaller(const std::string& requestReplyId) override;

    void addRequestCaller(const std::string& participantId,
                          std::shared_ptr<RequestCaller> requestCaller) override;

    void removeRequestCaller(const std::string& participantId) override;

    void receive(std::shared_ptr<ImmutableMessage> message) override;

    void registerSubscriptionManager(
            std::shared_ptr<ISubscriptionManager> subscriptionManager) override;

    void registerPublicationManager(std::weak_ptr<PublicationManager> publicationManager) override;

    void shutdown() override;

private:
    void handleRequestReceived(const ImmutableMessage& message);
    void handleOneWayRequestReceived(const ImmutableMessage& message);
    void handleReplyReceived(const ImmutableMessage& message);
    void handleMulticastReceived(const ImmutableMessage& message);
    void handlePublicationReceived(const ImmutableMessage& message);
    void handleSubscriptionRequestReceived(const ImmutableMessage& message);
    void handleBroadcastSubscriptionRequestReceived(const ImmutableMessage& message);
    void handleSubscriptionStopReceived(const ImmutableMessage& message);
    void handleSubscriptionReplyReceived(const ImmutableMessage& message);
    void handleMulticastSubscriptionRequestReceived(const ImmutableMessage& message);

private:
    DISALLOW_COPY_AND_ASSIGN(Dispatcher);
    std::shared_ptr<IMessageSender> messageSender;
    RequestCallerDirectory requestCallerDirectory;
    ReplyCallerDirectory replyCallerDirectory;
    std::weak_ptr<PublicationManager> publicationManager;
    std::shared_ptr<ISubscriptionManager> subscriptionManager;
    std::shared_ptr<ThreadPool> handleReceivedMessageThreadPool;
    ADD_LOGGER(Dispatcher);
    std::mutex subscriptionHandlingMutex;

    friend class ReceivedMessageRunnable;
};

} // namespace joynr
#endif // DISPATCHER_H
