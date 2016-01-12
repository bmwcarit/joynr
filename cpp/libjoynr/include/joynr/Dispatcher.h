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
/*
 * Dispatcher.h
 *
 *  Created on: Aug 9, 2011
 *      Author: grape
 */

#ifndef DISPATCHER_H_
#define DISPATCHER_H_
#include "joynr/PrivateCopyAssign.h"
#include "joynr/JoynrExport.h"

#include "joynr/IDispatcher.h"
#include "joynr/LibJoynrDirectories.h"
#include "joynr/Logger.h"
#include "joynr/ThreadPool.h"

#include <string>
#include <memory>

namespace joynr
{

class IReplyCaller;
class MessagingQos;
class RequestCaller;
class JoynrMessage;
class JoynrMessageSender;

class JOYNR_EXPORT Dispatcher : public IDispatcher
{

public:
    // ownership of messageSender is not passed to dispatcher, so dispatcher is not responsible for
    // deleting it.
    // Todo: should be changed to QSP or reference.
    explicit Dispatcher(JoynrMessageSender* messageSender, int maxThreads = 4);

    ~Dispatcher() override;

    void addReplyCaller(const std::string& requestReplyId,
                        std::shared_ptr<IReplyCaller> replyCaller,
                        const MessagingQos& qosSettings) override;

    void removeReplyCaller(const std::string& requestReplyId) override;

    void addRequestCaller(const std::string& participantId,
                          std::shared_ptr<RequestCaller> requestCaller) override;

    void removeRequestCaller(const std::string& participantId) override;

    void receive(const JoynrMessage& message) override;

    void registerSubscriptionManager(ISubscriptionManager* subscriptionManager) override;

    void registerPublicationManager(PublicationManager* publicationManager) override;

private:
    void handleRequestReceived(const JoynrMessage& message);
    void handleReplyReceived(const JoynrMessage& message);
    void handlePublicationReceived(const JoynrMessage& message);
    void handleSubscriptionRequestReceived(const JoynrMessage& message);
    void handleBroadcastSubscriptionRequestReceived(const JoynrMessage& message);
    void handleSubscriptionStopReceived(const JoynrMessage& message);

private:
    DISALLOW_COPY_AND_ASSIGN(Dispatcher);
    JoynrMessageSender* messageSender;
    RequestCallerDirectory requestCallerDirectory;
    ReplyCallerDirectory replyCallerDirectory;
    PublicationManager* publicationManager;
    ISubscriptionManager* subscriptionManager;
    ThreadPool handleReceivedMessageThreadPool;
    ADD_LOGGER(Dispatcher);
    std::mutex subscriptionHandlingMutex;

    friend class ReceivedMessageRunnable;
};

} // namespace joynr
#endif // DISPATCHER_H_
