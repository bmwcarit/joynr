/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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

#ifndef UDSMULTICASTADDRESSCALCULATOR_H
#define UDSMULTICASTADDRESSCALCULATOR_H

#include <memory>
#include <vector>

#include "joynr/IMulticastAddressCalculator.h"

namespace joynr
{

class ImmutableMessage;

namespace system
{
namespace RoutingTypes
{
class UdsAddress;
class Address;
} // namespace RoutingTypes
} // namespace system

class UdsMulticastAddressCalculator : public IMulticastAddressCalculator
{
public:
    explicit UdsMulticastAddressCalculator(
            std::shared_ptr<const system::RoutingTypes::UdsAddress> clusterControllerAddress);

    std::vector<std::shared_ptr<const system::RoutingTypes::Address>> compute(
            const ImmutableMessage& message) override;

private:
    std::shared_ptr<const system::RoutingTypes::UdsAddress> _clusterControllerAddress;
};

} // namespace joynr
#endif // UDSMULTICASTADDRESSCALCULATOR_H
