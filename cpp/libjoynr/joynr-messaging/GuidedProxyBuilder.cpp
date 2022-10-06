/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
#include "joynr/GuidedProxyBuilder.h"

namespace joynr
{

GuidedProxyBuilder::GuidedProxyBuilder(
        std::weak_ptr<JoynrRuntimeImpl> runtime,
        ProxyFactory& proxyFactory,
        std::weak_ptr<joynr::system::IDiscoveryAsync> discoveryProxy,
        const std::string& domain,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> dispatcherAddress,
        std::shared_ptr<IMessageRouter> messageRouter,
        MessagingSettings& messagingSettings,
        std::string interfaceName)
        : _runtime(std::move(runtime)),
          _proxyFactory(proxyFactory),
          _discoveryProxy(discoveryProxy),
          _dispatcherAddress(dispatcherAddress),
          _messageRouter(messageRouter),
          _domain(domain),
          _messagingSettings(messagingSettings),
          _discoveryCompletedOnce(false),
          _discoveryDefaultTimeoutMs(messagingSettings.getDiscoveryDefaultTimeoutMs()),
          _discoveryDefaultRetryIntervalMs(messagingSettings.getDiscoveryDefaultRetryIntervalMs()),
          _discoveryQos(),
          _messagingMaximumTtlMs(messagingSettings.getMaximumTtlMs()),
          _messagingQos(),
          _gbids(),
          _savedArbitrationResult(),
          _arbitrator(),
          _arbitratorMutex(),
          _interfaceName(interfaceName),
          _shuttingDown(false)
{
    _discoveryQos.setDiscoveryTimeoutMs(_discoveryDefaultTimeoutMs);
    _discoveryQos.setRetryIntervalMs(_discoveryDefaultRetryIntervalMs);
}

const std::string GuidedProxyBuilder::_runtimeAlreadyDestroyed =
        "required runtime has been already destroyed";

void GuidedProxyBuilder::stop()
{
    std::lock_guard<std::mutex> lock(_arbitratorMutex);
    _shuttingDown = true;
    if (_arbitrator) {
        _arbitrator->stopArbitration();
        _arbitrator.reset();
    }
}

std::shared_ptr<GuidedProxyBuilder> GuidedProxyBuilder::setMessagingQos(
        const MessagingQos& messagingQos) noexcept
{
    this->_messagingQos = messagingQos;
    if (this->_messagingQos.getTtl() > _messagingMaximumTtlMs) {
        this->_messagingQos.setTtl(_messagingMaximumTtlMs);
    }
    return this->shared_from_this();
}

std::shared_ptr<GuidedProxyBuilder> GuidedProxyBuilder::setDiscoveryQos(
        const DiscoveryQos& discoveryQos) noexcept
{
    this->_discoveryQos = discoveryQos;
    if (this->_discoveryQos.getDiscoveryTimeoutMs() == DiscoveryQos::NO_VALUE()) {
        this->_discoveryQos.setDiscoveryTimeoutMs(_discoveryDefaultTimeoutMs);
    }
    if (this->_discoveryQos.getRetryIntervalMs() == DiscoveryQos::NO_VALUE()) {
        this->_discoveryQos.setRetryIntervalMs(_discoveryDefaultRetryIntervalMs);
    }
    return this->shared_from_this();
}

std::shared_ptr<GuidedProxyBuilder> GuidedProxyBuilder::setGbids(
        const std::vector<std::string>& gbids)
{
    if (gbids.size() == 0) {
        throw std::invalid_argument("GBIDs vector must not be empty.");
    }
    this->_gbids = gbids;
    return this->shared_from_this();
}

DiscoveryResult GuidedProxyBuilder::discover()
{
    auto runtimeSharedPtr = _runtime.lock();
    if (runtimeSharedPtr == nullptr) {
        throw exceptions::DiscoveryException(_runtimeAlreadyDestroyed);
    }

    auto discoveryResultFuture = discoverAsync();

    DiscoveryResult discoveryResult;
    try {
        discoveryResultFuture->get(discoveryResult);
    } catch (const exceptions::DiscoveryException& discoveryException) {
        JOYNR_LOG_ERROR(logger(), "Discovery failed: {}", discoveryException.getMessage());
        throw discoveryException;
    }

    return discoveryResult;
}

std::shared_ptr<joynr::Future<joynr::DiscoveryResult>> GuidedProxyBuilder::discoverAsync()
{
    auto runtimeSharedPtr = _runtime.lock();
    std::lock_guard<std::mutex> lock(_arbitratorMutex);

    auto future = std::make_shared<joynr::Future<joynr::DiscoveryResult>>();

    if (runtimeSharedPtr == nullptr || _shuttingDown) {
        const exceptions::DiscoveryException error(_runtimeAlreadyDestroyed);
        future->onError(std::make_shared<exceptions::DiscoveryException>(error));
    }

    std::shared_ptr<GuidedProxyBuilder> thisSharedPtr = this->shared_from_this();
    auto onArbitrationSuccess = [thisWeakPtr = joynr::util::as_weak_ptr(thisSharedPtr),
                                 this,
                                 future](const ArbitrationResult& arbitrationResult) {
        auto proxyBuilderSharedPtr = thisWeakPtr.lock();
        if (proxyBuilderSharedPtr == nullptr) {
            const exceptions::DiscoveryException error(_runtimeAlreadyDestroyed);
            future->onError(std::make_shared<exceptions::DiscoveryException>(error));
        }
        // Convert arbitrationResult to discoveryResult
        const DiscoveryResult discoveryResult =
                createDiscoveryResultFromArbitrationResult(arbitrationResult);
        // Save discovered ArbitrationResult
        _savedArbitrationResult = arbitrationResult;
        // discover called once true
        _discoveryCompletedOnce = true;
        future->onSuccess(discoveryResult);
    };

    auto onArbitrationError = [thisWeakPtr = joynr::util::as_weak_ptr(thisSharedPtr),
                               future](const exceptions::DiscoveryException& exception) {
        auto proxyBuilderSharedPtr = thisWeakPtr.lock();
        if (proxyBuilderSharedPtr == nullptr) {
            const exceptions::DiscoveryException error(_runtimeAlreadyDestroyed);
            future->onError(std::make_shared<exceptions::DiscoveryException>(error));
        }
        future->onError(std::make_shared<exceptions::DiscoveryException>(exception));
    };

    // Create arbitrator
    _arbitrator = ArbitratorFactory::createArbitrator(_domain,
                                                      _interfaceName,
                                                      joynr::types::Version(),
                                                      _discoveryProxy,
                                                      _discoveryQos,
                                                      _gbids);

    _arbitrator->startArbitration(
            std::move(onArbitrationSuccess), std::move(onArbitrationError), false);

    return future;
}

DiscoveryResult GuidedProxyBuilder::createDiscoveryResultFromArbitrationResult(
        const ArbitrationResult& arbitrationResult)
{
    return DiscoveryResult(arbitrationResult.getDiscoveryEntries());
}

bool GuidedProxyBuilder::checkProviderAndProxyCompatibility(
        const joynr::types::Version& providerVersion,
        const types::Version& proxyVersion)
{
    return (providerVersion.getMajorVersion() == proxyVersion.getMajorVersion() &&
            providerVersion.getMinorVersion() >= proxyVersion.getMinorVersion());
}

} // namespace joynr
