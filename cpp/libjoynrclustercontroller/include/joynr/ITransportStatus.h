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
#ifndef ITRANSPORTSTATUS_H
#define ITRANSPORTSTATUS_H

#include <memory>
#include <functional>

namespace joynr
{
namespace system
{
namespace RoutingTypes
{
class Address;
} // RoutingTypes
} // namespace system

class ITransportStatus
{
public:
    virtual ~ITransportStatus() = default;

    virtual bool isReponsibleFor(std::shared_ptr<const joynr::system::RoutingTypes::Address>) = 0;
    virtual bool isAvailable() = 0;

    virtual void setAvailabilityChangedCallback(
            std::function<void(bool)> availabilityChangedCallback) = 0;
};
} // namespace joynr

#endif // ITRANSPORTSTATUS_H
