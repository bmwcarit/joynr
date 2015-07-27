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
#include "joynr/StdBroadcastFilterParameters.h"

namespace joynr
{

StdBroadcastFilterParameters::StdBroadcastFilterParameters()
        : filterParameters(std::map<std::string, std::string>())
{
}

StdBroadcastFilterParameters::StdBroadcastFilterParameters(
        const StdBroadcastFilterParameters& filterParameters)
        : filterParameters(filterParameters.filterParameters)
{
}

StdBroadcastFilterParameters& StdBroadcastFilterParameters::operator=(
        const StdBroadcastFilterParameters& filterParameters)
{
    this->filterParameters = filterParameters.filterParameters;
    return *this;
}

StdBroadcastFilterParameters::~StdBroadcastFilterParameters()
{
}

bool StdBroadcastFilterParameters::operator==(
        const StdBroadcastFilterParameters& filterParameters) const
{
    return filterParameters.getFilterParameters().size() == this->filterParameters.size() &&
           std::equal(this->filterParameters.begin(),
                      this->filterParameters.end(),
                      filterParameters.getFilterParameters().begin());
}

void StdBroadcastFilterParameters::setFilterParameter(const std::string& parameter,
                                                      const std::string& value)
{
    filterParameters.insert(std::pair<std::string, std::string>(parameter, value));
}

std::map<std::string, std::string> StdBroadcastFilterParameters::getFilterParameters() const
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

std::string StdBroadcastFilterParameters::getFilterParameter(const std::string& parameter) const
{
    std::map<std::string, std::string>::const_iterator iterator = filterParameters.find(parameter);
    if (iterator != filterParameters.end()) {
        return iterator->second;
    } else {
        return std::string();
    }
}

void StdBroadcastFilterParameters::setFilterParameters(
        const std::map<std::string, std::string>& value)
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
