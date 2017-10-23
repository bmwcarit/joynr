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
#include "joynr/InProcessDispatcher.h"

#include <cassert>
#include <tuple>

#include <boost/asio/io_service.hpp>

#include "joynr/MessagingQos.h"

namespace joynr
{

InProcessDispatcher::InProcessDispatcher(boost::asio::io_service& ioService)
        : requestCallerDirectory("InProcessDispatcher-RequestCallerDirectory", ioService),
          replyCallerDirectory("InProcessDispatcher-ReplyCallerDirectory", ioService),
          publicationManager(),
          subscriptionManager()
{
}

InProcessDispatcher::~InProcessDispatcher()
{
    JOYNR_LOG_TRACE(logger(), "Deleting InProcessDispatcher");
}

void InProcessDispatcher::addReplyCaller(const std::string& requestReplyId,
                                         std::shared_ptr<IReplyCaller> replyCaller,
                                         const MessagingQos& qosSettings)
{
    replyCallerDirectory.add(requestReplyId, replyCaller, qosSettings.getTtl());
}

void InProcessDispatcher::removeReplyCaller(const std::string& requestReplyId)
{
    replyCallerDirectory.remove(requestReplyId);
}

void InProcessDispatcher::addRequestCaller(const std::string& participantId,
                                           std::shared_ptr<RequestCaller> requestCaller)
{
    requestCallerDirectory.add(participantId, requestCaller);
    // check if there are subscriptions waiting for this provider
    // publicationManager->restore(participantId,requestCaller, messageSender);
}

void InProcessDispatcher::removeRequestCaller(const std::string& participantId)
{
    requestCallerDirectory.remove(participantId);
}

void InProcessDispatcher::receive(std::shared_ptr<ImmutableMessage> message)
{
    std::ignore = message;
    JOYNR_LOG_FATAL(logger(), "Not implemented");
    assert(false);
}

std::shared_ptr<RequestCaller> InProcessDispatcher::lookupRequestCaller(
        const std::string& participantId)
{
    return requestCallerDirectory.lookup(participantId);
}

bool InProcessDispatcher::containsRequestCaller(const std::string& participantId)
{
    return requestCallerDirectory.contains(participantId);
}

void InProcessDispatcher::registerSubscriptionManager(
        std::shared_ptr<ISubscriptionManager> subscriptionManager)
{
    this->subscriptionManager = subscriptionManager;
}

void InProcessDispatcher::registerPublicationManager(
        std::weak_ptr<PublicationManager> publicationManager)
{
    this->publicationManager = std::move(publicationManager);
}

void InProcessDispatcher::shutdown()
{
    requestCallerDirectory.shutdown();
    replyCallerDirectory.shutdown();
}

} // namespace joynr
