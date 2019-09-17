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
#include "joynr/BroadcastFilterParameters.h"

namespace joynr
{

BroadcastFilterParameters::BroadcastFilterParameters() : _filterParameters()
{
}

bool BroadcastFilterParameters::operator==(const BroadcastFilterParameters& other) const
{
    return this->_filterParameters == other._filterParameters;
}

void BroadcastFilterParameters::setFilterParameter(const std::string& parameter,
                                                   const std::string& value)
{
    _filterParameters.insert({parameter, value});
}

const std::map<std::string, std::string>& BroadcastFilterParameters::getFilterParameters() const
{
    return _filterParameters;
}

std::string BroadcastFilterParameters::getFilterParameter(const std::string& parameter) const
{
    std::map<std::string, std::string>::const_iterator it = _filterParameters.find(parameter);
    if (it != _filterParameters.cend()) {
        return it->second;
    } else {
        return std::string();
    }
}

void BroadcastFilterParameters::setFilterParameters(const std::map<std::string, std::string>& value)
{
    _filterParameters = value;
}

} // namespace joynr
