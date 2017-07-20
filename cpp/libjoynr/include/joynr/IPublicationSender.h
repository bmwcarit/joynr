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
#ifndef IPUBLICATIONSENDER_H
#define IPUBLICATIONSENDER_H

#include <string>

#include "joynr/JoynrExport.h"

namespace joynr
{

class SubscriptionPublication;
class SubscriptionReply;
class MessagingQos;

/**
  * @class IPublicationSender
  * @brief
  */
class JOYNR_EXPORT IPublicationSender
{
public:
    virtual ~IPublicationSender() = default;

    virtual void sendSubscriptionPublication(const std::string& senderParticipantId,
                                             const std::string& receiverParticipantId,
                                             const MessagingQos& qos,
                                             SubscriptionPublication&& subscriptionPublication) = 0;

    virtual void sendSubscriptionReply(const std::string& senderParticipantId,
                                       const std::string& receiverParticipantId,
                                       const MessagingQos& qos,
                                       const SubscriptionReply& subscriptionReply) = 0;
};

} // namespace joynr
#endif // IPUBLICATIONSENDER_H
