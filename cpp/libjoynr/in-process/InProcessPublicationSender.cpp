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
#include "joynr/InProcessPublicationSender.h"

#include <cassert>
#include <tuple>

#include "joynr/MetaTypeRegistrar.h"
#include "joynr/IPublicationInterpreter.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/ISubscriptionManager.h"

namespace joynr
{

INIT_LOGGER(InProcessPublicationSender);

InProcessPublicationSender::InProcessPublicationSender(ISubscriptionManager* subscriptionManager)
        : subscriptionManager(subscriptionManager)
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

    const std::string subscriptionId = subscriptionPublication.getSubscriptionId();
    JOYNR_LOG_TRACE(logger, "Sending publication. id={}", subscriptionId);
    assert(subscriptionManager != nullptr);
    subscriptionManager->touchSubscriptionState(subscriptionId);
    std::shared_ptr<ISubscriptionCallback> callback =
            subscriptionManager->getSubscriptionCallback(subscriptionId);
    if (!callback) {
        JOYNR_LOG_ERROR(logger,
                        "Dropping reply for non/no more existing subscription with id={}",
                        subscriptionId);
        return;
    }

    int typeId = callback->getTypeId();

    // Get the publication interpreter - this has to be a reference to support
    // PublicationInterpreter polymorphism
    IPublicationInterpreter& interpreter =
            MetaTypeRegistrar::instance().getPublicationInterpreter(typeId);
    JOYNR_LOG_TRACE(logger, "Interpreting publication. id={}", subscriptionId);
    interpreter.execute(callback, std::move(subscriptionPublication));
}

} // namespace joynr
