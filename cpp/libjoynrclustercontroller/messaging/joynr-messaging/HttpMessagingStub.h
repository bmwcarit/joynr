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
#ifndef HTTPMESSAGINGSTUB_H
#define HTTPMESSAGINGSTUB_H

#include <memory>

#include "joynr/IMessagingStub.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"

namespace joynr
{

class ITransportMessageSender;

/**
  * Is used by the ClusterController to contact another (remote) ClusterController
  */
class HttpMessagingStub : public IMessagingStub
{
public:
    explicit HttpMessagingStub(std::shared_ptr<ITransportMessageSender> messageSender,
                               const system::RoutingTypes::ChannelAddress& destinationAddress);
    ~HttpMessagingStub() override = default;
    void transmit(std::shared_ptr<ImmutableMessage> message,
                  const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure)
            override;

private:
    DISALLOW_COPY_AND_ASSIGN(HttpMessagingStub);
    std::shared_ptr<ITransportMessageSender> _messageSender;
    const system::RoutingTypes::ChannelAddress _destinationAddress;

    ADD_LOGGER(HttpMessagingStub)
};

} // namespace joynr
#endif // HTTPMESSAGINGSTUB_H
