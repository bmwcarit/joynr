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
#include "joynr/PrivateCopyAssign.h"
#include "joynr/ProxyFactory.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/system/IDiscovery.h"
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"

namespace joynr
{

class ICapabilities;

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
class ProxyBuilder : public IProxyBuilder<T>
{
public:
    /**
     * @brief Constructor
     * @param proxyFactory Pointer to proxy factory object
     * @param discoveryProxy weak ptr to IDiscoverySync object
     * @param domain The provider domain
     * @param dispatcherAddress The address of the dispatcher
     * @param messageRouter A shared pointer to the message router object
     */
    ProxyBuilder(ProxyFactory& proxyFactory,
                 IRequestCallerDirectory* requestCallerDirectory,
                 std::weak_ptr<joynr::system::IDiscoveryAsync> discoveryProxy,
                 const std::string& domain,
                 std::shared_ptr<const joynr::system::RoutingTypes::Address> dispatcherAddress,
                 std::shared_ptr<IMessageRouter> messageRouter,
                 std::uint64_t messagingMaximumTtlMs);

    /** Destructor */
    ~ProxyBuilder() override = default;

    /**
     * @brief Build the proxy object
     *
     * The proxy is build and returned to the caller. The caller takes ownership of the proxy and
     * is responsible for deletion.
     * @return The proxy object
     */
    std::unique_ptr<T> build() override;

    /**
     * @brief Build the proxy object asynchronously
     *
     * @param onSucess: Will be invoked when building the proxy succeeds. The created proxy is
     * passed as the parameter.
     * @param onError: Will be invoked when the proxy could not be created. An exception, which
     * describes the error, is passed as the parameter.
     */
    void buildAsync(std::function<void(std::unique_ptr<T> proxy)> onSuccess,
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

    std::string domain;
    MessagingQos messagingQos;
    ProxyFactory& proxyFactory;
    IRequestCallerDirectory* requestCallerDirectory;
    std::weak_ptr<joynr::system::IDiscoveryAsync> discoveryProxy;
    std::unique_ptr<Arbitrator> arbitrator;

    std::shared_ptr<const joynr::system::RoutingTypes::Address> dispatcherAddress;
    std::shared_ptr<IMessageRouter> messageRouter;
    std::uint64_t messagingMaximumTtlMs;
    DiscoveryQos discoveryQos;

    ADD_LOGGER(ProxyBuilder);
};

template <class T>
ProxyBuilder<T>::ProxyBuilder(
        ProxyFactory& proxyFactory,
        IRequestCallerDirectory* requestCallerDirectory,
        std::weak_ptr<system::IDiscoveryAsync> discoveryProxy,
        const std::string& domain,
        std::shared_ptr<const system::RoutingTypes::Address> dispatcherAddress,
        std::shared_ptr<IMessageRouter> messageRouter,
        std::uint64_t messagingMaximumTtlMs)
        : domain(domain),
          messagingQos(),
          proxyFactory(proxyFactory),
          requestCallerDirectory(requestCallerDirectory),
          discoveryProxy(discoveryProxy),
          arbitrator(),
          dispatcherAddress(dispatcherAddress),
          messageRouter(messageRouter),
          messagingMaximumTtlMs(messagingMaximumTtlMs),
          discoveryQos()
{
}

template <class T>
std::unique_ptr<T> ProxyBuilder<T>::build()
{
    Future<std::unique_ptr<T>> proxyFuture;

    auto onSuccess =
            [&proxyFuture](std::unique_ptr<T> proxy) { proxyFuture.onSuccess(std::move(proxy)); };

    auto onError = [&proxyFuture](const exceptions::DiscoveryException& exception) {
        proxyFuture.onError(std::make_shared<exceptions::DiscoveryException>(exception));
    };

    buildAsync(onSuccess, onError);

    std::unique_ptr<T> createdProxy;
    proxyFuture.get(createdProxy);

    return createdProxy;
}

template <class T>
void ProxyBuilder<T>::buildAsync(
        std::function<void(std::unique_ptr<T> proxy)> onSuccess,
        std::function<void(const exceptions::DiscoveryException& exception)> onError)
{
    joynr::types::Version interfaceVersion(T::MAJOR_VERSION, T::MINOR_VERSION);
    arbitrator = ArbitratorFactory::createArbitrator(
            domain, T::INTERFACE_NAME(), interfaceVersion, discoveryProxy, discoveryQos);

    auto arbitrationSucceeds =
            [this, onSuccess, onError](const types::DiscoveryEntryWithMetaInfo& discoverEntry) {
        if (discoverEntry.getParticipantId().empty()) {
            onError(exceptions::DiscoveryException("Arbitration was set to successfull by "
                                                   "arbitrator but ParticipantId is empty"));
            return;
        }

        JOYNR_LOG_DEBUG(logger,
                        "DISCOVERY proxy created for provider participantId: {}, domain: [{}], "
                        "interface: {}",
                        discoverEntry.getParticipantId(),
                        domain,
                        T::INTERFACE_NAME());

        bool useInProcessConnector =
                requestCallerDirectory->containsRequestCaller(discoverEntry.getParticipantId());
        std::unique_ptr<T> proxy(proxyFactory.createProxy<T>(domain, messagingQos));
        proxy->handleArbitrationFinished(discoverEntry, useInProcessConnector);

        messageRouter->addNextHop(proxy->getProxyParticipantId(), dispatcherAddress);

        onSuccess(std::move(proxy));
    };

    arbitrator->startArbitration(arbitrationSucceeds, onError);
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
    return this;
}

template <class T>
INIT_LOGGER(ProxyBuilder<T>);

} // namespace joynr
#endif // PROXYBUILDER_H
