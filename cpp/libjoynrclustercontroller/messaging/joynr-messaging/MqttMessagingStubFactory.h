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
#ifndef MQTTMESSAGINGSTUBFACTORY_H
#define MQTTMESSAGINGSTUBFACTORY_H

#include <functional>
#include <memory>
#include <string>

#include "joynr/IMiddlewareMessagingStubFactory.h"
#include "joynr/Logger.h"

namespace joynr
{

class IMessagingStub;
class ITransportMessageSender;

namespace system
{
namespace RoutingTypes
{
class Address;
}
} // namespace system

class MqttMessagingStubFactory : public IMiddlewareMessagingStubFactory
{

public:
    MqttMessagingStubFactory(std::shared_ptr<ITransportMessageSender> messageSender,
                             const std::string& gbid);
    std::shared_ptr<IMessagingStub> create(
            const joynr::system::RoutingTypes::Address& destAddress) override;
    bool canCreate(const joynr::system::RoutingTypes::Address& destAddress) override;
    void registerOnMessagingStubClosedCallback(
            std::function<void(std::shared_ptr<const joynr::system::RoutingTypes::Address>
                                       destinationAddress)> onMessagingStubClosedCallback) override;

private:
    std::shared_ptr<ITransportMessageSender> _messageSender;
    const std::string _gbid;
    ADD_LOGGER(MqttMessagingStubFactory)
};

} // namespace joynr
#endif // MQTTMESSAGINGSTUBFACTORY_H
