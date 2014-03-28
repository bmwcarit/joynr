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
#include "libjoynr/joynr-messaging/JoynrMessagingStub.h"
#include "joynr/ICommunicationManager.h"
#include "joynr/MessagingQos.h"
#include "joynr/JoynrMessage.h"

namespace joynr {

JoynrMessagingStub::JoynrMessagingStub(
        ICommunicationManager& communicationManager,
        QString destinationChannelId):
    communicationManager(communicationManager),
    destinationChannelId(destinationChannelId)
{}

JoynrMessagingStub::~JoynrMessagingStub() {}

void JoynrMessagingStub::transmit(JoynrMessage& message, const MessagingQos& qos) {
    if (message.getType() == JoynrMessage::VALUE_MESSAGE_TYPE_REQUEST || message.getType() == JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST){
        message.setHeaderReplyChannelId(communicationManager.getReceiveChannelId());
    }
    communicationManager.sendMessage(destinationChannelId, qos.getTtl(),message);
}

} // namespace joynr
