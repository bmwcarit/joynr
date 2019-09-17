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

#include "joynr/DiscoveryQos.h"

namespace joynr
{

DiscoveryQos::ArbitrationStrategy& DiscoveryQos::DEFAULT_ARBITRATIONSTRATEGY()
{
    static DiscoveryQos::ArbitrationStrategy default_strategy =
            DiscoveryQos::ArbitrationStrategy::LAST_SEEN;
    return default_strategy;
}

std::int64_t DiscoveryQos::DO_NOT_USE_CACHE()
{
    return 0;
}

std::int64_t DiscoveryQos::DEFAULT_CACHEMAXAGE_MS()
{
    return DO_NOT_USE_CACHE();
}

joynr::types::DiscoveryScope::Enum& DiscoveryQos::DEFAULT_DISCOVERYSCOPE()
{
    static joynr::types::DiscoveryScope::Enum default_scope =
            joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL;
    return default_scope;
}

DiscoveryQos::DiscoveryQos()
        : _customParameters(),
          _arbitrationStrategy(DEFAULT_ARBITRATIONSTRATEGY()),
          _discoveryTimeoutMs(NO_VALUE()),
          _cacheMaxAgeMs(DEFAULT_CACHEMAXAGE_MS()),
          _discoveryScope(DEFAULT_DISCOVERYSCOPE()),
          _providerMustSupportOnChange(false),
          _retryIntervalMs(NO_VALUE())
{
}

DiscoveryQos::DiscoveryQos(std::int64_t cacheMaxAgeMs)
        : _customParameters(),
          _arbitrationStrategy(DEFAULT_ARBITRATIONSTRATEGY()),
          _discoveryTimeoutMs(NO_VALUE()),
          _cacheMaxAgeMs(cacheMaxAgeMs),
          _discoveryScope(DEFAULT_DISCOVERYSCOPE()),
          _providerMustSupportOnChange(false),
          _retryIntervalMs(NO_VALUE())
{
}

std::int64_t DiscoveryQos::NO_VALUE()
{
    return -1L;
}

void DiscoveryQos::setArbitrationStrategy(ArbitrationStrategy arbitrationStrategy)
{
    this->_arbitrationStrategy = arbitrationStrategy;
}

void DiscoveryQos::setDiscoveryTimeoutMs(std::int64_t discoveryTimeoutMs)
{
    this->_discoveryTimeoutMs = discoveryTimeoutMs;
    if (this->_discoveryTimeoutMs < 0) {
        this->_discoveryTimeoutMs = 0;
    }
}

std::int64_t DiscoveryQos::getDiscoveryTimeoutMs() const
{
    return _discoveryTimeoutMs;
}

DiscoveryQos::ArbitrationStrategy DiscoveryQos::getArbitrationStrategy() const
{
    return _arbitrationStrategy;
}

void DiscoveryQos::addCustomParameter(std::string name, std::string value)
{
    types::CustomParameter param(name, value);
    _customParameters[name] = param;
}

types::CustomParameter DiscoveryQos::getCustomParameter(std::string name) const
{
    return _customParameters.at(name);
}

std::map<std::string, types::CustomParameter> DiscoveryQos::getCustomParameters() const
{
    return _customParameters;
}

std::int64_t DiscoveryQos::getCacheMaxAgeMs() const
{
    return _cacheMaxAgeMs;
}

void DiscoveryQos::setCacheMaxAgeMs(const std::int64_t cacheMaxAgeMs)
{
    this->_cacheMaxAgeMs = cacheMaxAgeMs;
}

bool DiscoveryQos::getProviderMustSupportOnChange() const
{
    return _providerMustSupportOnChange;
}

void DiscoveryQos::setProviderMustSupportOnChange(bool providerMustSupportOnChange)
{
    this->_providerMustSupportOnChange = providerMustSupportOnChange;
}

const std::string DiscoveryQos::KEYWORD_PARAMETER()
{
    static const std::string keyword("keyword");
    return keyword;
}

joynr::types::DiscoveryScope::Enum DiscoveryQos::getDiscoveryScope() const
{
    return _discoveryScope;
}

void DiscoveryQos::setDiscoveryScope(joynr::types::DiscoveryScope::Enum discoveryScope)
{
    this->_discoveryScope = discoveryScope;
}

std::int64_t DiscoveryQos::getRetryIntervalMs() const
{
    return this->_retryIntervalMs;
}

void DiscoveryQos::setRetryIntervalMs(std::int64_t retryIntervalMs)
{
    this->_retryIntervalMs = retryIntervalMs;
}

} // namespace joynr
