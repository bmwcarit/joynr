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
#ifndef PROXYBUILDER_H
#define PROXYBUILDER_H

#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "joynr/Arbitrator.h"
#include "joynr/ArbitratorFactory.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/Future.h"
#include "joynr/IMessageRouter.h"
#include "joynr/IProxyBuilder.h"
#include "joynr/IRequestCallerDirectory.h"
#include "joynr/LocalDiscoveryAggregator.h"
#include "joynr/Logger.h"
#include "joynr/MessagingQos.h"
#include "joynr/MessagingSettings.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/ProxyFactory.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/system/IDiscovery.h"
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"

namespace joynr
{

class ICapabilities;
class JoynrRuntimeImpl;
class MessagingSettings;

/**
 * @brief Class to build a proxy object for the given interface T.
 *
 * Default proxy properties can be overwritten by the set...Qos methods.
 * After calling build the proxy can be used like a local instance of the provider.
 * All invocations will be queued until either the message TTL expires or the
 * arbitration finishes successfully. Synchronous calls will block until the
 * arbitration is done.
 */
template <class T>
class ProxyBuilder : public IProxyBuilder<T>, public std::enable_shared_from_this<ProxyBuilder<T>>
{
public:
    /**
     * @brief Constructor
     * @param proxyFactory Pointer to proxy factory object
     * @param discoveryProxy weak ptr to IDiscoverySync object
     * @param domain The provider domain
     * @param dispatcherAddress The address of the dispatcher
     * @param messageRouter A shared pointer to the message router object
     * @param messagingSettings Reference to the messaging settings object
     */
    ProxyBuilder(std::weak_ptr<JoynrRuntimeImpl> _runtime,
                 ProxyFactory& _proxyFactory,
                 std::shared_ptr<IRequestCallerDirectory> _requestCallerDirectory,
                 std::weak_ptr<joynr::system::IDiscoveryAsync> _discoveryProxy,
                 const std::string& _domain,
                 std::shared_ptr<const joynr::system::RoutingTypes::Address> _dispatcherAddress,
                 std::shared_ptr<IMessageRouter> _messageRouter,
                 MessagingSettings& messagingSettings);

    /** Destructor */
    ~ProxyBuilder() override = default;

    /**
     * @brief Build the proxy object
     *
     * The proxy is build and returned to the caller. The caller takes ownership of the proxy and
     * is responsible for deletion.
     * @return The proxy object
     */
    std::shared_ptr<T> build() override;

    void stop() override
    {
        std::lock_guard<std::mutex> lock(_arbitratorsMutex);
        _shuttingDown = true;
        for (auto arbitrator : _arbitrators) {
            arbitrator->stopArbitration();
            arbitrator.reset();
        }
        _arbitrators.clear();
    }

    /**
     * @brief Build the proxy object asynchronously
     *
     * @param onSucess: Will be invoked when building the proxy succeeds. The created proxy is
     * passed as the parameter.
     * @param onError: Will be invoked when the proxy could not be created. An exception, which
     * describes the error, is passed as the parameter.
     */
    void buildAsync(
            std::function<void(std::shared_ptr<T> proxy)> onSuccess,
            std::function<void(const exceptions::DiscoveryException&)> onError) noexcept override;

    /**
     * @brief OPTIONAL - Sets the messaging Qos settings. If no messaging Qos is provided, a default
     * one will be used (see MessagingQos.h).
     * @param messagingQos The message quality of service settings
     * @return The ProxyBuilder object
     */
    std::shared_ptr<IProxyBuilder<T>> setMessagingQos(
            const MessagingQos& _messagingQos) noexcept override;

    /**
     * @brief OPTIONAL - Sets the discovery Qos settings. If no discovery Qos is provided, a default
     * one will be used based on the default-messaging.setting file.
     * @param discoveryQos The discovery quality of service settings
     * @return The ProxyBuilder object
     */
    std::shared_ptr<IProxyBuilder<T>> setDiscoveryQos(
            const DiscoveryQos& _discoveryQos) noexcept override;

    /**
     * @brief Sets the GBIDs (Global Backend Identifiers) to select the backends in which the
     * provider will be discovered.<br>
     * Global discovery (if enabled in DiscoveryQos) will be done via the
     * GlobalCapabilitiesDirectory in the backend of the first provided GBID.<br>
     * By default, providers will be discovered in all backends known to the cluster controller via
     * the GlobalCapabilitiesDirectory in the default backend.
     * @param gbids A vector of GBIDs
     * @return The ProxyBuilder object
     * @throw std::invalid_argument if provided gbids vector is empty
     */
    std::shared_ptr<IProxyBuilder<T>> setGbids(const std::vector<std::string>& _gbids) override;

private:
    DISALLOW_COPY_AND_ASSIGN(ProxyBuilder);

    std::weak_ptr<JoynrRuntimeImpl> _runtime;
    ProxyFactory& _proxyFactory;
    std::shared_ptr<IRequestCallerDirectory> _requestCallerDirectory;
    std::weak_ptr<joynr::system::IDiscoveryAsync> _discoveryProxy;
    std::vector<std::shared_ptr<Arbitrator>> _arbitrators;
    std::mutex _arbitratorsMutex;
    bool _shuttingDown;
    std::shared_ptr<const joynr::system::RoutingTypes::Address> _dispatcherAddress;
    std::shared_ptr<IMessageRouter> _messageRouter;

    std::string _domain;
    std::uint64_t _messagingMaximumTtlMs;
    MessagingQos _messagingQos;
    std::int64_t _discoveryDefaultTimeoutMs;
    std::int64_t _discoveryDefaultRetryIntervalMs;
    DiscoveryQos _discoveryQos;
    std::vector<std::string> _gbids;

    static const std::string _runtimeAlreadyDestroyed;

    ADD_LOGGER(ProxyBuilder)
};

template <class T>
ProxyBuilder<T>::ProxyBuilder(
        std::weak_ptr<JoynrRuntimeImpl> runtime,
        ProxyFactory& proxyFactory,
        std::shared_ptr<IRequestCallerDirectory> requestCallerDirectory,
        std::weak_ptr<system::IDiscoveryAsync> discoveryProxy,
        const std::string& domain,
        std::shared_ptr<const system::RoutingTypes::Address> dispatcherAddress,
        std::shared_ptr<IMessageRouter> messageRouter,
        MessagingSettings& messagingSettings)
        : _runtime(std::move(runtime)),
          _proxyFactory(proxyFactory),
          _requestCallerDirectory(requestCallerDirectory),
          _discoveryProxy(discoveryProxy),
          _arbitrators(),
          _arbitratorsMutex(),
          _shuttingDown(false),
          _dispatcherAddress(dispatcherAddress),
          _messageRouter(messageRouter),
          _domain(domain),
          _messagingMaximumTtlMs(messagingSettings.getMaximumTtlMs()),
          _messagingQos(),
          _discoveryDefaultTimeoutMs(messagingSettings.getDiscoveryDefaultTimeoutMs()),
          _discoveryDefaultRetryIntervalMs(messagingSettings.getDiscoveryDefaultRetryIntervalMs()),
          _discoveryQos(),
          _gbids()
{
    _discoveryQos.setDiscoveryTimeoutMs(_discoveryDefaultTimeoutMs);
    _discoveryQos.setRetryIntervalMs(_discoveryDefaultRetryIntervalMs);
}

template <class T>
const std::string ProxyBuilder<T>::_runtimeAlreadyDestroyed =
        "required runtime has been already destroyed";

template <class T>
std::shared_ptr<T> ProxyBuilder<T>::build()
{
    auto runtimeSharedPtr = _runtime.lock();
    if (runtimeSharedPtr == nullptr) {
        throw exceptions::DiscoveryException(_runtimeAlreadyDestroyed);
    }
    auto proxyFuture = std::make_shared<Future<std::shared_ptr<T>>>();

    auto onSuccess =
            [proxyFuture](std::shared_ptr<T> proxy) { proxyFuture->onSuccess(std::move(proxy)); };

    auto onError = [proxyFuture](const exceptions::DiscoveryException& exception) {
        proxyFuture->onError(std::make_shared<exceptions::DiscoveryException>(exception));
    };

    buildAsync(std::move(onSuccess), std::move(onError));

    std::shared_ptr<T> createdProxy;
    proxyFuture->get(createdProxy);

    return createdProxy;
}

template <class T>
void ProxyBuilder<T>::buildAsync(
        std::function<void(std::shared_ptr<T> proxy)> onSuccess,
        std::function<void(const exceptions::DiscoveryException& exception)> onError) noexcept
{
    auto runtimeSharedPtr = _runtime.lock();
    std::lock_guard<std::mutex> lock(_arbitratorsMutex);

    if (runtimeSharedPtr == nullptr || _shuttingDown) {
        const exceptions::DiscoveryException error(_runtimeAlreadyDestroyed);
        onError(error);
    }

    joynr::types::Version interfaceVersion(T::MAJOR_VERSION, T::MINOR_VERSION);

    std::shared_ptr<ProxyBuilder<T>> thisSharedPtr = this->shared_from_this();
    auto arbitrationSucceeds = [
        thisWeakPtr = joynr::util::as_weak_ptr(thisSharedPtr),
        this,
        onSuccess = std::move(onSuccess),
        onError
    ](const types::DiscoveryEntryWithMetaInfo& discoverEntry) mutable
    {
        // need to make sure own instance still exists before
        // accesssing internal inherited member runtime
        auto proxyBuilderSharedPtr = thisWeakPtr.lock();
        if (proxyBuilderSharedPtr == nullptr) {
            onError(exceptions::DiscoveryException(_runtimeAlreadyDestroyed));
            return;
        }
        auto runtimeSharedPtr = _runtime.lock();
        if (runtimeSharedPtr == nullptr) {
            onError(exceptions::DiscoveryException(_runtimeAlreadyDestroyed));
            return;
        }

        if (discoverEntry.getParticipantId().empty()) {
            onError(exceptions::DiscoveryException("Arbitration was set to successfull by "
                                                   "arbitrator but ParticipantId is empty"));
            return;
        }

        std::shared_ptr<T> proxy =
                _proxyFactory.createProxy<T>(runtimeSharedPtr, _domain, _messagingQos);
        proxy->handleArbitrationFinished(discoverEntry);

        JOYNR_LOG_INFO(logger(),
                       "DISCOVERY proxy: participantId {} created for provider participantId: {}, "
                       "domain: [{}], "
                       "interface: {}",
                       proxy->getProxyParticipantId(),
                       discoverEntry.getParticipantId(),
                       _domain,
                       T::INTERFACE_NAME());

        bool isGloballyVisible = !discoverEntry.getIsLocal();
        constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
        const bool isSticky = false;
        auto onSuccessAddNextHop = [onSuccess, proxy]() { onSuccess(std::move(proxy)); };
        auto onErrorAddNextHop = [onError](
                const joynr::exceptions::ProviderRuntimeException& providerRuntimeException) {
            if (onError) {
                onError(exceptions::DiscoveryException(
                        "Proxy could not be added to parent router: " +
                        providerRuntimeException.getMessage()));
            }
        };

        _messageRouter->setToKnown(discoverEntry.getParticipantId());
        _messageRouter->addNextHop(proxy->getProxyParticipantId(),
                                   _dispatcherAddress,
                                   isGloballyVisible,
                                   expiryDateMs,
                                   isSticky,
                                   onSuccessAddNextHop,
                                   onErrorAddNextHop);
    };

    auto arbitrator = ArbitratorFactory::createArbitrator(
            _domain, T::INTERFACE_NAME(), interfaceVersion, _discoveryProxy, _discoveryQos, _gbids);
    arbitrator->startArbitration(std::move(arbitrationSucceeds), std::move(onError));
    _arbitrators.push_back(std::move(arbitrator));
}

template <class T>
std::shared_ptr<IProxyBuilder<T>> ProxyBuilder<T>::setMessagingQos(
        const MessagingQos& messagingQos) noexcept
{
    this->_messagingQos = messagingQos;
    // check validity of messaging maximum TTL
    if (this->_messagingQos.getTtl() > _messagingMaximumTtlMs) {
        this->_messagingQos.setTtl(_messagingMaximumTtlMs);
    }
    return this->shared_from_this();
}

template <class T>
/* Sets the arbitration Qos and starts arbitration. This way arbitration will be started, before the
   ->build() is called on the proxy Builder.
   All parameter that are needed for arbitration should be set, before setDiscoveryQos is called.
*/
std::shared_ptr<IProxyBuilder<T>> ProxyBuilder<T>::setDiscoveryQos(
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

template <class T>
std::shared_ptr<IProxyBuilder<T>> ProxyBuilder<T>::setGbids(const std::vector<std::string>& gbids)
{
    if (gbids.size() == 0) {
        throw std::invalid_argument("GBIDs vector must not be empty.");
    }
    this->_gbids = gbids;
    return this->shared_from_this();
}

} // namespace joynr
#endif // PROXYBUILDER_H
