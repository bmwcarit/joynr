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

#include "joynr/DiscoveryQos.h"
#include "joynr/exceptions.h"

namespace joynr
{

int64_t& DiscoveryQos::DEFAULT_DISCOVERYTIMEOUT()
{
    static int64_t default_timeout = 30000;
    return default_timeout;
}

DiscoveryQos::ArbitrationStrategy& DiscoveryQos::DEFAULT_ARBITRATIONSTRATEGY()
{
    static DiscoveryQos::ArbitrationStrategy default_strategy =
            DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY;
    return default_strategy;
}

int64_t& DiscoveryQos::DO_NOT_USE_CACHE()
{
    static int64_t do_not_use_cache = 0;
    return do_not_use_cache;
}

int64_t& DiscoveryQos::DEFAULT_CACHEMAXAGE()
{
    return DO_NOT_USE_CACHE();
}

joynr::types::DiscoveryScope::Enum& DiscoveryQos::DEFAULT_DISCOVERYSCOPE()
{
    static joynr::types::DiscoveryScope::Enum default_scope =
            joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL;
    return default_scope;
}

int64_t& DiscoveryQos::DEFAULT_RETRYINTERVAL()
{
    static int64_t default_retryInterval = 1000;
    return default_retryInterval;
}

DiscoveryQos::DiscoveryQos()
        : customParameters(),
          arbitrationStrategy(DEFAULT_ARBITRATIONSTRATEGY()),
          discoveryTimeout(DEFAULT_DISCOVERYTIMEOUT()),
          cacheMaxAge(DEFAULT_CACHEMAXAGE()),
          discoveryScope(DEFAULT_DISCOVERYSCOPE()),
          providerMustSupportOnChange(false),
          retryInterval(DEFAULT_RETRYINTERVAL())
{
}

DiscoveryQos::DiscoveryQos(const int64_t& cacheMaxAge)
        : customParameters(),
          arbitrationStrategy(DEFAULT_ARBITRATIONSTRATEGY()),
          discoveryTimeout(DEFAULT_DISCOVERYTIMEOUT()),
          cacheMaxAge(cacheMaxAge),
          discoveryScope(DEFAULT_DISCOVERYSCOPE()),
          providerMustSupportOnChange(false),
          retryInterval(DEFAULT_RETRYINTERVAL())
{
}

void DiscoveryQos::setArbitrationStrategy(ArbitrationStrategy arbitrationStrategy)
{
    this->arbitrationStrategy = arbitrationStrategy;
}

void DiscoveryQos::setDiscoveryTimeout(int64_t discoveryTimeout)
{
    this->discoveryTimeout = discoveryTimeout;
    if (this->discoveryTimeout < 0) {
        this->discoveryTimeout = 0;
    }
}

int64_t DiscoveryQos::getDiscoveryTimeout() const
{
    return discoveryTimeout;
}

DiscoveryQos::ArbitrationStrategy DiscoveryQos::getArbitrationStrategy() const
{
    return arbitrationStrategy;
}

void DiscoveryQos::addCustomParameter(std::string name, std::string value)
{
    types::CustomParameter param(name, value);
    customParameters[name] = param;
}

types::CustomParameter DiscoveryQos::getCustomParameter(std::string name) const
{
    return customParameters.at(name);
}

std::map<std::string, types::CustomParameter> DiscoveryQos::getCustomParameters() const
{
    return customParameters;
}

int64_t DiscoveryQos::getCacheMaxAge() const
{
    return cacheMaxAge;
}

void DiscoveryQos::setCacheMaxAge(const int64_t& cacheMaxAge)
{
    this->cacheMaxAge = cacheMaxAge;
}

bool DiscoveryQos::getProviderMustSupportOnChange() const
{
    return providerMustSupportOnChange;
}

void DiscoveryQos::setProviderMustSupportOnChange(bool providerMustSupportOnChange)
{
    this->providerMustSupportOnChange = providerMustSupportOnChange;
}

const std::string DiscoveryQos::KEYWORD_PARAMETER()
{
    static const std::string keyword("keyword");
    return keyword;
}

joynr::types::DiscoveryScope::Enum DiscoveryQos::getDiscoveryScope() const
{
    return discoveryScope;
}

void DiscoveryQos::setDiscoveryScope(joynr::types::DiscoveryScope::Enum discoveryScope)
{
    this->discoveryScope = discoveryScope;
}

int64_t DiscoveryQos::getRetryInterval() const
{
    return this->retryInterval;
}

void DiscoveryQos::setRetryInterval(int64_t retryInterval)
{
    this->retryInterval = retryInterval;
}

} // namespace joynr
