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
#include "joynr/ICapabilities.h"

namespace joynr {

qint64& DiscoveryQos::DEFAULT_DISCOVERYTIMEOUT(){
    static qint64 default_timeout = 30000;
    return default_timeout;
}

DiscoveryQos::ArbitrationStrategy& DiscoveryQos::DEFAULT_ARBITRATIONSTRATEGY() {
    static DiscoveryQos::ArbitrationStrategy default_strategy = DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY;
    return default_strategy;
}

qint64& DiscoveryQos::DO_NOT_USE_CACHE(){
    static qint64 do_not_use_cache = 0;
    return do_not_use_cache;
}

qint64& DiscoveryQos::DEFAULT_CACHEMAXAGE(){
    return DO_NOT_USE_CACHE();
}

DiscoveryQos::DiscoveryScope& DiscoveryQos::DEFAULT_DISCOVERYSCOPE() {
    static DiscoveryQos::DiscoveryScope default_scope = DiscoveryQos::DiscoveryScope::LOCAL_THEN_GLOBAL;
    return default_scope;
}

qint64& DiscoveryQos::DEFAULT_RETRYINTERVAL() {
    static qint64 default_retryInterval = 1000;
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

DiscoveryQos::DiscoveryQos(const qint64& cacheMaxAge)
    : customParameters(),
      arbitrationStrategy(DEFAULT_ARBITRATIONSTRATEGY()),
      discoveryTimeout (DEFAULT_DISCOVERYTIMEOUT()),
      cacheMaxAge(cacheMaxAge),
      discoveryScope(DEFAULT_DISCOVERYSCOPE()),
      providerMustSupportOnChange(false),
      retryInterval(DEFAULT_RETRYINTERVAL())
{
}

void DiscoveryQos::setArbitrationStrategy(ArbitrationStrategy arbitrationStrategy){
   this->arbitrationStrategy = arbitrationStrategy;
}

void DiscoveryQos::setDiscoveryTimeout(qint64 discoveryTimeout){
    this->discoveryTimeout  = discoveryTimeout;
    if(this->discoveryTimeout < 0) {
        this->discoveryTimeout = 0;
    }
}

qint64 DiscoveryQos::getDiscoveryTimeout() const {
    return discoveryTimeout;
}

DiscoveryQos::ArbitrationStrategy DiscoveryQos::getArbitrationStrategy() const {
    return arbitrationStrategy;
}

void DiscoveryQos::addCustomParameter(QString name, QString value){
    types::CustomParameter param(name, value);
    customParameters.insert(name, param);
}

types::CustomParameter DiscoveryQos::getCustomParameter(QString name) const {
    return customParameters.value(name);
}

QMap<QString, types::CustomParameter> DiscoveryQos::getCustomParameters() const {
    return customParameters;
}

qint64 DiscoveryQos::getCacheMaxAge() const {
    return cacheMaxAge;
}

void DiscoveryQos::setCacheMaxAge(const qint64& cacheMaxAge) {
    this->cacheMaxAge = cacheMaxAge;
    if(this->cacheMaxAge < 0) {
        this->cacheMaxAge = 0;
    }
}

bool DiscoveryQos::getProviderMustSupportOnChange() const {
    return providerMustSupportOnChange;
}

void DiscoveryQos::setProviderMustSupportOnChange(bool providerMustSupportOnChange) {
    this->providerMustSupportOnChange = providerMustSupportOnChange ;
}

const QString DiscoveryQos::KEYWORD_PARAMETER() {
    static const QString keyword("keyword");
    return keyword;
}

DiscoveryQos::DiscoveryScope DiscoveryQos::getDiscoveryScope() const {
    return discoveryScope;
}

void DiscoveryQos::setDiscoveryScope(DiscoveryScope discoveryScope) {
    this->discoveryScope = discoveryScope;
}

qint64 DiscoveryQos::getRetryInterval() const {
    return this->retryInterval;
}

void DiscoveryQos::setRetryInterval(qint64 retryInterval) {
    this->retryInterval = retryInterval;
    if(this->retryInterval < 0) {
        this->retryInterval = 0;
    }
}

} // namespace joynr
