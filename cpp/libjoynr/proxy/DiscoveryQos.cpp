/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

std::int64_t& DiscoveryQos::DEFAULT_DISCOVERYTIMEOUT_MS()
{
    static std::int64_t default_timeout = 30000;
    return default_timeout;
}

DiscoveryQos::ArbitrationStrategy& DiscoveryQos::DEFAULT_ARBITRATIONSTRATEGY()
{
    static DiscoveryQos::ArbitrationStrategy default_strategy =
            DiscoveryQos::ArbitrationStrategy::LAST_SEEN;
    return default_strategy;
}

std::int64_t& DiscoveryQos::DO_NOT_USE_CACHE()
{
    static std::int64_t do_not_use_cache = 0;
    return do_not_use_cache;
}

std::int64_t& DiscoveryQos::DEFAULT_CACHEMAXAGE_MS()
{
    return DO_NOT_USE_CACHE();
}

joynr::types::DiscoveryScope::Enum& DiscoveryQos::DEFAULT_DISCOVERYSCOPE()
{
    static joynr::types::DiscoveryScope::Enum default_scope =
            joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL;
    return default_scope;
}

std::int64_t& DiscoveryQos::DEFAULT_RETRYINTERVAL_MS()
{
    static std::int64_t default_retryInterval = 1000;
    return default_retryInterval;
}

DiscoveryQos::DiscoveryQos()
        : customParameters(),
          arbitrationStrategy(DEFAULT_ARBITRATIONSTRATEGY()),
          discoveryTimeoutMs(DEFAULT_DISCOVERYTIMEOUT_MS()),
          cacheMaxAgeMs(DEFAULT_CACHEMAXAGE_MS()),
          discoveryScope(DEFAULT_DISCOVERYSCOPE()),
          providerMustSupportOnChange(false),
          retryIntervalMs(DEFAULT_RETRYINTERVAL_MS())
{
}

DiscoveryQos::DiscoveryQos(const std::int64_t& cacheMaxAge)
        : customParameters(),
          arbitrationStrategy(DEFAULT_ARBITRATIONSTRATEGY()),
          discoveryTimeoutMs(DEFAULT_DISCOVERYTIMEOUT_MS()),
          cacheMaxAgeMs(cacheMaxAge),
          discoveryScope(DEFAULT_DISCOVERYSCOPE()),
          providerMustSupportOnChange(false),
          retryIntervalMs(DEFAULT_RETRYINTERVAL_MS())
{
}

void DiscoveryQos::setArbitrationStrategy(ArbitrationStrategy arbitrationStrategy)
{
    this->arbitrationStrategy = arbitrationStrategy;
}

void DiscoveryQos::setDiscoveryTimeoutMs(std::int64_t discoveryTimeoutMs)
{
    this->discoveryTimeoutMs = discoveryTimeoutMs;
    if (this->discoveryTimeoutMs < 0) {
        this->discoveryTimeoutMs = 0;
    }
}

std::int64_t DiscoveryQos::getDiscoveryTimeoutMs() const
{
    return discoveryTimeoutMs;
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

std::int64_t DiscoveryQos::getCacheMaxAgeMs() const
{
    return cacheMaxAgeMs;
}

void DiscoveryQos::setCacheMaxAgeMs(const std::int64_t& cacheMaxAgeMs)
{
    this->cacheMaxAgeMs = cacheMaxAgeMs;
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

std::int64_t DiscoveryQos::getRetryIntervalMs() const
{
    return this->retryIntervalMs;
}

void DiscoveryQos::setRetryIntervalMs(std::int64_t retryIntervalMs)
{
    this->retryIntervalMs = retryIntervalMs;
}

} // namespace joynr
