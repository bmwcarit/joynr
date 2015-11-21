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

BroadcastFilterParameters::BroadcastFilterParameters()
        : filterParameters(std::map<std::string, std::string>())
{
}

BroadcastFilterParameters::BroadcastFilterParameters(
        const BroadcastFilterParameters& filterParameters)
        : filterParameters(filterParameters.filterParameters)
{
}

BroadcastFilterParameters& BroadcastFilterParameters::operator=(
        const BroadcastFilterParameters& filterParameters)
{
    this->filterParameters = filterParameters.filterParameters;
    return *this;
}

BroadcastFilterParameters::~BroadcastFilterParameters()
{
}

bool BroadcastFilterParameters::operator==(const BroadcastFilterParameters& filterParameters) const
{
    return filterParameters.getFilterParameters().size() == this->filterParameters.size() &&
           std::equal(this->filterParameters.begin(),
                      this->filterParameters.end(),
                      filterParameters.getFilterParameters().begin());
}

void BroadcastFilterParameters::setFilterParameter(const std::string& parameter,
                                                   const std::string& value)
{
    filterParameters.insert(std::pair<std::string, std::string>(parameter, value));
}

std::map<std::string, std::string> BroadcastFilterParameters::getFilterParameters() const
{
    std::map<std::string, std::string> fiterParameters;
    for (std::map<std::string, std::string>::const_iterator iterator =
                 this->filterParameters.begin();
         iterator != this->filterParameters.end();
         iterator++) {
        fiterParameters.insert(
                std::pair<std::string, std::string>(iterator->first, iterator->second));
    }
    return fiterParameters;
}

std::string BroadcastFilterParameters::getFilterParameter(const std::string& parameter) const
{
    std::map<std::string, std::string>::const_iterator iterator = filterParameters.find(parameter);
    if (iterator != filterParameters.end()) {
        return iterator->second;
    } else {
        return std::string();
    }
}

void BroadcastFilterParameters::setFilterParameters(const std::map<std::string, std::string>& value)
{
    filterParameters.clear();
    for (std::map<std::string, std::string>::const_iterator iterator = value.begin();
         iterator != value.end();
         iterator++) {
        filterParameters.insert(
                std::pair<std::string, std::string>(iterator->first, iterator->second));
    }
}

} // namespace joynr
