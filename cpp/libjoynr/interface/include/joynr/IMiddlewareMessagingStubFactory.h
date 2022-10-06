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
#ifndef IMIDDLEWAREMESSAGINGSTUBFACTORY_H
#define IMIDDLEWAREMESSAGINGSTUBFACTORY_H

#include <functional>
#include <memory>

namespace joynr
{

namespace system
{

namespace RoutingTypes
{
class Address;
} // namespace RoutingTypes
} // namespace system
class IMessagingStub;

class IMiddlewareMessagingStubFactory
{
public:
    virtual ~IMiddlewareMessagingStubFactory() = default;
    virtual std::shared_ptr<IMessagingStub> create(
            const joynr::system::RoutingTypes::Address& destAddress) = 0;
    virtual bool canCreate(const joynr::system::RoutingTypes::Address& destAddress) = 0;
    virtual void registerOnMessagingStubClosedCallback(
            std::function<void(std::shared_ptr<const joynr::system::RoutingTypes::Address>
                                       destinationAddress)> onMessagingStubDisconnected) = 0;
};

} // namespace joynr

#endif // IMIDDLEWAREMESSAGINGSTUBFACTORY_H
