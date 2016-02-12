/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
#ifndef MQTTMESSAGINGSKELETON_H
#define MQTTMESSAGINGSKELETON_H

#include <string>

#include "joynr/IMessaging.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"

namespace joynr
{

class MessageRouter;
class JoynrMessage;

class MqttMessagingSkeleton : public IMessaging
{
public:
    explicit MqttMessagingSkeleton(MessageRouter& messageRouter);

    ~MqttMessagingSkeleton() override = default;

    void transmit(JoynrMessage& message) override;

    void onTextMessageReceived(const std::string& message);

private:
    DISALLOW_COPY_AND_ASSIGN(MqttMessagingSkeleton);
    ADD_LOGGER(MqttMessagingSkeleton);
    MessageRouter& messageRouter;
};

} // namespace joynr
#endif // MQTTMESSAGINGSKELETON_H
