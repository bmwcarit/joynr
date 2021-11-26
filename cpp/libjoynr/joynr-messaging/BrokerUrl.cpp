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

#include <iostream>
#include <sstream>
#include <string>

#include "joynr/BrokerUrl.h"

namespace joynr
{

BrokerUrl::BrokerUrl(const std::string& brokerBaseUrlString)
        : _brokerBaseUrlString(brokerBaseUrlString)
{
    this->_brokerBaseUrl = Url(brokerBaseUrlString);
    if (!this->_brokerBaseUrl.isValid()) {
        std::ostringstream errorStringStream;
        errorStringStream << "could not parse URL >" << brokerBaseUrlString << "<";
        throw std::invalid_argument(errorStringStream.str());
    }
}

BrokerUrl& BrokerUrl::operator=(const BrokerUrl& brokerUrl)
{
    _brokerBaseUrlString = brokerUrl._brokerBaseUrlString;
    _brokerBaseUrl = brokerUrl._brokerBaseUrl;
    return *this;
}

bool BrokerUrl::operator==(const BrokerUrl& brokerUrl) const
{
    return _brokerBaseUrl == brokerUrl.getBrokerBaseUrl();
}

Url BrokerUrl::getBrokerBaseUrl() const
{
    Url url(_brokerBaseUrl);
    return url;
}

std::string BrokerUrl::toString() const
{
    return _brokerBaseUrlString;
}

} // namespace joynr
