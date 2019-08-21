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

#ifndef IMULTICASTADDRESSCALCULATOR_H
#define IMULTICASTADDRESSCALCULATOR_H

#include <memory>
#include <vector>

#include "joynr/JoynrExport.h"
#include "joynr/system/RoutingTypes/Address.h"

namespace joynr
{

class ImmutableMessage;

class JOYNR_EXPORT IMulticastAddressCalculator
{
public:
    /*
     * Compute a multicast routing address for the given message.
     */
    virtual std::vector<std::shared_ptr<const joynr::system::RoutingTypes::Address>> compute(
            const ImmutableMessage& message) = 0;
    virtual ~IMulticastAddressCalculator() = default;
};

} // namespace joynr
#endif // IMULTICASTADDRESSCALCULATOR_H
