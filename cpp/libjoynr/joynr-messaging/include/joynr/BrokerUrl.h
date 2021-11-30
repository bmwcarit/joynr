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
#ifndef BROKERURL_H
#define BROKERURL_H

#include "joynr/JoynrExport.h"
#include "joynr/Url.h"

#include <string>

namespace joynr
{

class JOYNR_EXPORT BrokerUrl
{
public:
    explicit BrokerUrl(const std::string& brokerBaseUrlString);

    BrokerUrl(const BrokerUrl& other) = default;

    BrokerUrl& operator=(const BrokerUrl& brokerUrl);
    bool operator==(const BrokerUrl& brokerUrl) const;

    Url getBrokerBaseUrl() const;
    std::string toString() const;

private:
    std::string _brokerBaseUrlString;
    Url _brokerBaseUrl;
};

} // namespace joynr
#endif // BROKERURL_H
