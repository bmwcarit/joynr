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
#ifndef GUIDEDPROXYBUILDER_H
#define GUIDEDPROXYBUILDER_H

#include "joynr/Arbitrator.h"
#include "joynr/ArbitratorFactory.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/DiscoveryResult.h"
#include "joynr/Logger.h"
#include "joynr/MessagingQos.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/ProxyBuilder.h"

namespace joynr
{

class JoynrRuntimeImpl;

class GuidedProxyBuilder : public IProxyBuilderBase,
                           public std::enable_shared_from_this<GuidedProxyBuilder>
{
public:
    GuidedProxyBuilder(
            std::weak_ptr<JoynrRuntimeImpl> runtime,
            ProxyFactory& proxyFactory,
            std::weak_ptr<joynr::system::IDiscoveryAsync> discoveryProxy,
            const std::string& domain,
            std::shared_ptr<const joynr::system::RoutingTypes::Address> dispatcherAddress,
            std::shared_ptr<IMessageRouter> messageRouter,
            MessagingSettings& messagingSettings,
            std::string interfaceName);

    ~GuidedProxyBuilder() = default;

    std::shared_ptr<GuidedProxyBuilder> setMessagingQos(const MessagingQos& messagingQos) noexcept;

    std::shared_ptr<GuidedProxyBuilder> setDiscoveryQos(const DiscoveryQos& discoveryQos) noexcept;

    std::shared_ptr<GuidedProxyBuilder> setGbids(const std::vector<std::string>& gbids);

    void stop() override;

    DiscoveryResult discover();

    std::shared_ptr<joynr::Future<joynr::DiscoveryResult>> discoverAsync();

    template <class TProxy>
    std::shared_ptr<TProxy> buildProxy(const std::string& participantId);

private:
    DiscoveryResult createDiscoveryResultFromArbitrationResult(
            const ArbitrationResult& arbitrationResult);
    bool checkProviderAndProxyVersions(const types::Version& providerVersion,
                                       const types::Version& proxyVersion);

private:
    DISALLOW_COPY_AND_ASSIGN(GuidedProxyBuilder);

    // NOTE: necessary for ProxyBuilder creation
    std::weak_ptr<JoynrRuntimeImpl> _runtime;
    ProxyFactory& _proxyFactory;
    std::weak_ptr<joynr::system::IDiscoveryAsync> _discoveryProxy;
    std::shared_ptr<const joynr::system::RoutingTypes::Address> _dispatcherAddress;
    std::shared_ptr<IMessageRouter> _messageRouter;
    const std::string _domain;
    MessagingSettings _messagingSettings;

    bool _discoveryCompletedOnce;
    const std::int64_t _discoveryDefaultTimeoutMs;
    const std::int64_t _discoveryDefaultRetryIntervalMs;
    DiscoveryQos _discoveryQos;

    const std::uint64_t _messagingMaximumTtlMs;
    MessagingQos _messagingQos;

    std::vector<std::string> _gbids;

    ArbitrationResult _savedArbitrationResult;
    std::shared_ptr<Arbitrator> _arbitrator;
    std::mutex _arbitratorMutex;
    std::string _interfaceName;

    bool _shuttingDown;
    static const std::string _runtimeAlreadyDestroyed;

    ADD_LOGGER(GuidedProxyBuilder)
};

template <class TProxy>
std::shared_ptr<TProxy> GuidedProxyBuilder::buildProxy(const std::string& participantId)
{
    auto runtimeSharedPtr = _runtime.lock();
    if (runtimeSharedPtr == nullptr) {
        throw exceptions::DiscoveryException(_runtimeAlreadyDestroyed);
    }

    if (!_discoveryCompletedOnce) {
        throw exceptions::DiscoveryException(
                "Discovery has to be completed before building a proxy!");
    }

    bool discoveryFound = false;
    joynr::types::DiscoveryEntryWithMetaInfo discoveryEntry;
    for (const auto& entry : _savedArbitrationResult.getDiscoveryEntries()) {
        if (entry.getParticipantId() == participantId) {
            discoveryEntry = entry;
            discoveryFound = true;
        }
    }
    if (!discoveryFound) {
        throw exceptions::DiscoveryException("No provider with participant ID " + participantId +
                                             " was discovered!");
    }

    // Check version compatibility of proxy's interface and found discovery entry
    joynr::types::Version interfaceVersion =
            joynr::types::Version(TProxy::MAJOR_VERSION, TProxy::MINOR_VERSION);
    if (!checkProviderAndProxyVersions(discoveryEntry.getProviderVersion(), interfaceVersion)) {
        throw exceptions::DiscoveryException("Interface " + _interfaceName +
                                             " and provider with participant ID" + participantId +
                                             " have incompatible versions!");
    }

    std::vector<joynr::types::DiscoveryEntryWithMetaInfo> discoveryEntries{discoveryEntry};
    ArbitrationResult arbitrationResult = ArbitrationResult(discoveryEntries);
    auto proxyBuilder = std::make_shared<ProxyBuilder<TProxy>>(_runtime,
                                                               _proxyFactory,
                                                               _discoveryProxy,
                                                               _domain,
                                                               _dispatcherAddress,
                                                               _messageRouter,
                                                               _messagingSettings);
    proxyBuilder->setMessagingQos(_messagingQos);

    return proxyBuilder->build(arbitrationResult);
}

} // namespace joynr

#endif // GUIDEDPROXYBUILDER_H
