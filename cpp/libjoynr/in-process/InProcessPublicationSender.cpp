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
#include "joynr/InProcessPublicationSender.h"

#include <cassert>
#include <tuple>

#include "joynr/ISubscriptionCallback.h"
#include "joynr/ISubscriptionManager.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/SubscriptionReply.h"

namespace joynr
{

INIT_LOGGER(InProcessPublicationSender);

InProcessPublicationSender::InProcessPublicationSender(
        std::shared_ptr<ISubscriptionManager> subscriptionManager)
        : subscriptionManager(std::move(subscriptionManager))
{
}

void InProcessPublicationSender::sendSubscriptionPublication(
        const std::string& senderParticipantId,
        const std::string& receiverParticipantId,
        const MessagingQos& qos,
        SubscriptionPublication&& subscriptionPublication)
{
    std::ignore = senderParticipantId; // interface has sourcePartId, because JoynrMessages have a
                                       // source and dest. partId. Those are not necessary for in
                                       // process
    std::ignore = receiverParticipantId;
    std::ignore = qos;

    /**
      * just call the InProcessDispatcher!
      */

    const std::string& subscriptionId = subscriptionPublication.getSubscriptionId();
    JOYNR_LOG_DEBUG(logger, "Sending publication. id={}", subscriptionId);
    assert(subscriptionManager != nullptr);
    subscriptionManager->touchSubscriptionState(subscriptionId);
    std::shared_ptr<ISubscriptionCallback> callback =
            subscriptionManager->getSubscriptionCallback(subscriptionId);
    if (!callback) {
        JOYNR_LOG_ERROR(logger,
                        "Dropping subscription publication for non/no more existing subscription "
                        "with id={}",
                        subscriptionId);
        return;
    }

    callback->execute(std::move(subscriptionPublication));
}

void InProcessPublicationSender::sendSubscriptionReply(const std::string& senderParticipantId,
                                                       const std::string& receiverParticipantId,
                                                       const MessagingQos& qos,
                                                       const SubscriptionReply& subscriptionReply)
{
    std::ignore = senderParticipantId; // interface has sourcePartId, because JoynrMessages have a
                                       // source and dest. partId. Those are not necessary for in
                                       // process
    std::ignore = receiverParticipantId;
    std::ignore = qos;

    const std::string& subscriptionId = subscriptionReply.getSubscriptionId();
    JOYNR_LOG_DEBUG(logger, "Sending publication. id={}", subscriptionId);
    assert(subscriptionManager != nullptr);
    std::shared_ptr<ISubscriptionCallback> callback =
            subscriptionManager->getSubscriptionCallback(subscriptionId);
    if (!callback) {
        JOYNR_LOG_ERROR(
                logger,
                "Dropping subscription reply for non/no more existing subscription with id={}",
                subscriptionId);
        return;
    }

    callback->execute(std::move(subscriptionReply));
}

} // namespace joynr
