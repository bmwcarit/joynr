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
#include "joynr/BroadcastFilterParameters.h"

#include "joynr/Variant.h"

namespace joynr
{

// Register the BroadcastFilterParameters type id
static const bool isBroadcastFilterParametersRegistered =
        Variant::registerType<BroadcastFilterParameters>("joynr.BroadcastFilterParameters");

BroadcastFilterParameters::BroadcastFilterParameters() : filterParameters()
{
}

bool BroadcastFilterParameters::operator==(const BroadcastFilterParameters& other) const
{
    return this->filterParameters == other.filterParameters;
}

void BroadcastFilterParameters::setFilterParameter(const std::string& parameter,
                                                   const std::string& value)
{
    filterParameters.insert({parameter, value});
}

const std::map<std::string, std::string>& BroadcastFilterParameters::getFilterParameters() const
{
    return filterParameters;
}

std::string BroadcastFilterParameters::getFilterParameter(const std::string& parameter) const
{
    auto it = filterParameters.find(parameter);
    if (it != filterParameters.cend()) {
        return it->second;
    } else {
        return std::string();
    }
}

void BroadcastFilterParameters::setFilterParameters(const std::map<std::string, std::string>& value)
{
    filterParameters = value;
}

} // namespace joynr
