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

#include "joynr/Arbitrator.h"
#include "joynr/ArbitratorFactory.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/Future.h"
#include "joynr/IMessageRouter.h"
#include "joynr/IProxyBuilder.h"
#include "joynr/IRequestCallerDirectory.h"
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
class JoynrRuntime;
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
    ProxyBuilder(std::weak_ptr<JoynrRuntime> runtime,
                 ProxyFactory& proxyFactory,
                 std::shared_ptr<IRequestCallerDirectory> requestCallerDirectory,
                 std::weak_ptr<joynr::system::IDiscoveryAsync> discoveryProxy,
                 const std::string& domain,
                 std::shared_ptr<const joynr::system::RoutingTypes::Address> dispatcherAddress,
                 std::shared_ptr<IMessageRouter> messageRouter,
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

    /**
     * @brief Build the proxy object asynchronously
     *
     * @param onSucess: Will be invoked when building the proxy succeeds. The created proxy is
     * passed as the parameter.
     * @param onError: Will be invoked when the proxy could not be created. An exception, which
     * describes the error, is passed as the parameter.
     */
    void buildAsync(std::function<void(std::shared_ptr<T> proxy)> onSuccess,
                    std::function<void(const exceptions::DiscoveryException&)> onError) override;

    /**
     * @brief Sets the messaging qos settings
     * @param messagingQos The message quality of service settings
     * @return The ProxyBuilder object
     */
    ProxyBuilder* setMessagingQos(const MessagingQos& messagingQos) override;

    /**
     * @brief Sets the discovery qos settings
     * @param discoveryQos The discovery quality of service settings
     * @return The ProxyBuilder object
     */
    ProxyBuilder* setDiscoveryQos(const DiscoveryQos& discoveryQos) override;

private:
    DISALLOW_COPY_AND_ASSIGN(ProxyBuilder);

    std::weak_ptr<JoynrRuntime> runtime;
    std::string domain;
    MessagingQos messagingQos;
    ProxyFactory& proxyFactory;
    std::shared_ptr<IRequestCallerDirectory> requestCallerDirectory;
    std::weak_ptr<joynr::system::IDiscoveryAsync> discoveryProxy;
    std::shared_ptr<Arbitrator> arbitrator;

    std::shared_ptr<const joynr::system::RoutingTypes::Address> dispatcherAddress;
    std::shared_ptr<IMessageRouter> messageRouter;
    std::uint64_t messagingMaximumTtlMs;
    std::int64_t discoveryDefaultTimeoutMs;
    std::int64_t discoveryDefaultRetryIntervalMs;
    DiscoveryQos discoveryQos;
    static const std::string runtimeAlreadyDestroyed;

    ADD_LOGGER(ProxyBuilder)
};

template <class T>
ProxyBuilder<T>::ProxyBuilder(
        std::weak_ptr<JoynrRuntime> runtime,
        ProxyFactory& proxyFactory,
        std::shared_ptr<IRequestCallerDirectory> requestCallerDirectory,
        std::weak_ptr<system::IDiscoveryAsync> discoveryProxy,
        const std::string& domain,
        std::shared_ptr<const system::RoutingTypes::Address> dispatcherAddress,
        std::shared_ptr<IMessageRouter> messageRouter,
        MessagingSettings& messagingSettings)
        : runtime(std::move(runtime)),
          domain(domain),
          messagingQos(),
          proxyFactory(proxyFactory),
          requestCallerDirectory(requestCallerDirectory),
          discoveryProxy(discoveryProxy),
          arbitrator(),
          dispatcherAddress(dispatcherAddress),
          messageRouter(messageRouter),
          messagingMaximumTtlMs(messagingSettings.getMaximumTtlMs()),
          discoveryDefaultTimeoutMs(messagingSettings.getDiscoveryDefaultTimeoutMs()),
          discoveryDefaultRetryIntervalMs(messagingSettings.getDiscoveryDefaultRetryIntervalMs()),
          discoveryQos()
{
}

template <class T>
const std::string ProxyBuilder<T>::runtimeAlreadyDestroyed =
        "required runtime has been already destroyed";

template <class T>
std::shared_ptr<T> ProxyBuilder<T>::build()
{
    auto runtimeSharedPtr = runtime.lock();
    if (runtimeSharedPtr == nullptr) {
        throw exceptions::DiscoveryException(runtimeAlreadyDestroyed);
    }
    Future<std::shared_ptr<T>> proxyFuture;

    auto onSuccess =
            [&proxyFuture](std::shared_ptr<T> proxy) { proxyFuture.onSuccess(std::move(proxy)); };

    auto onError = [&proxyFuture](const exceptions::DiscoveryException& exception) {
        proxyFuture.onError(std::make_shared<exceptions::DiscoveryException>(exception));
    };

    buildAsync(std::move(onSuccess), std::move(onError));

    std::shared_ptr<T> createdProxy;
    proxyFuture.get(createdProxy);

    return createdProxy;
}

template <class T>
void ProxyBuilder<T>::buildAsync(
        std::function<void(std::shared_ptr<T> proxy)> onSuccess,
        std::function<void(const exceptions::DiscoveryException& exception)> onError)
{
    auto runtimeSharedPtr = runtime.lock();
    if (runtimeSharedPtr == nullptr) {
        throw exceptions::DiscoveryException(runtimeAlreadyDestroyed);
    }
    joynr::types::Version interfaceVersion(T::MAJOR_VERSION, T::MINOR_VERSION);
    arbitrator = ArbitratorFactory::createArbitrator(
            domain, T::INTERFACE_NAME(), interfaceVersion, discoveryProxy, discoveryQos);

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
            onError(exceptions::DiscoveryException(runtimeAlreadyDestroyed));
            return;
        }
        auto runtimeSharedPtr = runtime.lock();
        if (runtimeSharedPtr == nullptr) {
            onError(exceptions::DiscoveryException(runtimeAlreadyDestroyed));
            return;
        }

        if (discoverEntry.getParticipantId().empty()) {
            onError(exceptions::DiscoveryException("Arbitration was set to successfull by "
                                                   "arbitrator but ParticipantId is empty"));
            return;
        }

        JOYNR_LOG_DEBUG(logger(),
                        "DISCOVERY proxy created for provider participantId: {}, domain: [{}], "
                        "interface: {}",
                        discoverEntry.getParticipantId(),
                        domain,
                        T::INTERFACE_NAME());

        bool useInProcessConnector =
                requestCallerDirectory->containsRequestCaller(discoverEntry.getParticipantId());
        std::shared_ptr<T> proxy =
                proxyFactory.createProxy<T>(runtimeSharedPtr, domain, messagingQos);
        proxy->handleArbitrationFinished(discoverEntry, useInProcessConnector);

        bool isGloballyVisible = !discoverEntry.getIsLocal();
        constexpr std::int64_t expiryDateMs = std::numeric_limits<std::int64_t>::max();
        const bool isSticky = false;
        messageRouter->addNextHop(proxy->getProxyParticipantId(),
                                  dispatcherAddress,
                                  isGloballyVisible,
                                  expiryDateMs,
                                  isSticky);

        onSuccess(std::move(proxy));
    };

    arbitrator->startArbitration(std::move(arbitrationSucceeds), std::move(onError));
}

template <class T>
ProxyBuilder<T>* ProxyBuilder<T>::setMessagingQos(const MessagingQos& messagingQos)
{
    this->messagingQos = messagingQos;
    // check validity of messaging maximum TTL
    if (this->messagingQos.getTtl() > messagingMaximumTtlMs) {
        this->messagingQos.setTtl(messagingMaximumTtlMs);
    }
    return this;
}

template <class T>
/* Sets the arbitration Qos and starts arbitration. This way arbitration will be started, before the
   ->build() is called on the proxy Builder.
   All parameter that are needed for arbitration should be set, before setDiscoveryQos is called.
*/
ProxyBuilder<T>* ProxyBuilder<T>::setDiscoveryQos(const DiscoveryQos& discoveryQos)
{
    this->discoveryQos = discoveryQos;
    if (this->discoveryQos.getDiscoveryTimeoutMs() == DiscoveryQos::NO_VALUE()) {
        this->discoveryQos.setDiscoveryTimeoutMs(discoveryDefaultTimeoutMs);
    }
    if (this->discoveryQos.getRetryIntervalMs() == DiscoveryQos::NO_VALUE()) {
        this->discoveryQos.setRetryIntervalMs(discoveryDefaultRetryIntervalMs);
    }
    return this;
}

} // namespace joynr
#endif // PROXYBUILDER_H
