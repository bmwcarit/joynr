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
#include <mutex>
#include <string>

#include "joynr/BoostIoserviceForwardDecl.h"
#include "joynr/IDispatcher.h"
#include "joynr/JoynrExport.h"
#include "joynr/LibJoynrDirectories.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/ReadWriteLock.h"

namespace joynr
{

class IMessageSender;
class IReplyCaller;
class ISubscriptionManager;
class ImmutableMessage;
class MessagingQos;
class PublicationManager;
class RequestCaller;
class ThreadPool;

class JOYNR_EXPORT Dispatcher : public std::enable_shared_from_this<Dispatcher>, public IDispatcher
{

public:
    Dispatcher(std::shared_ptr<IMessageSender> messageSender, boost::asio::io_service& ioService);

    ~Dispatcher() override;

    void addReplyCaller(const std::string& requestReplyId,
                        std::shared_ptr<IReplyCaller> replyCaller,
                        const MessagingQos& qosSettings) override;

    void addRequestCaller(const std::string& participantId,
                          std::shared_ptr<RequestCaller> requestCaller) override;

    void removeRequestCaller(const std::string& participantId) override;

    void receive(std::shared_ptr<ImmutableMessage> message) override;

    void registerSubscriptionManager(
            std::shared_ptr<ISubscriptionManager> subscriptionManager) override;

    void registerPublicationManager(std::weak_ptr<PublicationManager> publicationManager) override;

    void shutdown() override;

private:
    void handleRequestReceived(std::shared_ptr<ImmutableMessage> message);
    void handleOneWayRequestReceived(std::shared_ptr<ImmutableMessage> message);
    void handleReplyReceived(std::shared_ptr<ImmutableMessage> message);
    void handleMulticastReceived(std::shared_ptr<ImmutableMessage> message);
    void handlePublicationReceived(std::shared_ptr<ImmutableMessage> message);
    void handleSubscriptionRequestReceived(std::shared_ptr<ImmutableMessage> message);
    void handleBroadcastSubscriptionRequestReceived(std::shared_ptr<ImmutableMessage> message);
    void handleSubscriptionStopReceived(std::shared_ptr<ImmutableMessage> message);
    void handleSubscriptionReplyReceived(std::shared_ptr<ImmutableMessage> message);
    void handleMulticastSubscriptionRequestReceived(std::shared_ptr<ImmutableMessage> message);

private:
    DISALLOW_COPY_AND_ASSIGN(Dispatcher);
    std::shared_ptr<IMessageSender> _messageSender;
    RequestCallerDirectory _requestCallerDirectory;
    ReplyCallerDirectory _replyCallerDirectory;
    std::weak_ptr<PublicationManager> _publicationManager;
    std::shared_ptr<ISubscriptionManager> _subscriptionManager;
    std::shared_ptr<ThreadPool> _handleReceivedMessageThreadPool;
    ADD_LOGGER(Dispatcher)
    std::mutex _subscriptionHandlingMutex;
    bool _isShuttingDown;
    ReadWriteLock _isShuttingDownLock;

    friend class ReceivedMessageRunnable;
};

} // namespace joynr
#endif // DISPATCHER_H
