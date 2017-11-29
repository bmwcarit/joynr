/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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

#ifndef SHORTCIRCUITRUNTIME_H
#define SHORTCIRCUITRUNTIME_H

#include <memory>
#include <tuple> // for std::ignore

#include "joynr/CapabilityUtils.h"
#include "joynr/ClusterControllerSettings.h"
#include "joynr/IMessageRouter.h"
#include "joynr/InProcessPublicationSender.h"
#include "joynr/IPlatformSecurityManager.h"
#include "joynr/JoynrRuntime.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/types/ProviderQos.h"

namespace joynr
{

class IKeychain;
class IMessageSender;
class InProcessMessagingSkeleton;
class Settings;
class SubscriptionManager;

class DummyDiscovery : public joynr::system::IDiscoveryAsync
{

private:
    template <typename T, typename F>
    auto resolve(T value, F onSuccess)
    {
        auto future = std::make_shared<Future<T>>();
        future->onSuccess(value);
        if (onSuccess) {
            onSuccess(value);
        }
        return future;
    }

    template <typename F>
    auto resolve(F onSuccess)
    {
        auto future = std::make_shared<Future<void>>();
        future->onSuccess();
        if (onSuccess) {
            onSuccess();
        }
        return future;
    }

public:
    std::shared_ptr<joynr::Future<void>> addAsync(
            const joynr::types::DiscoveryEntry& discoveryEntry,
            std::function<void()> onSuccess = nullptr,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)>
                    onRuntimeError = nullptr) override
    {
        std::ignore = onRuntimeError;
        entry = discoveryEntry;
        return resolve(onSuccess);
    }

    std::shared_ptr<joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>
    lookupAsync(
            const std::vector<std::string>& domains,
            const std::string& interfaceName,
            const joynr::types::DiscoveryQos& discoveryQos,
            std::function<void(const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& result)>
                    onSuccess = nullptr,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)>
                    onRuntimeError = nullptr) override
    {
        std::vector<joynr::types::DiscoveryEntryWithMetaInfo> result;
        result.push_back(joynr::util::convert(true, entry));
        return resolve(result, onSuccess);
    }

    std::shared_ptr<joynr::Future<joynr::types::DiscoveryEntryWithMetaInfo>> lookupAsync(
            const std::string& participantId,
            std::function<void(const joynr::types::DiscoveryEntryWithMetaInfo& result)> onSuccess =
                    nullptr,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)>
                    onRuntimeError = nullptr) override
    {
        return resolve(joynr::util::convert(true, entry), onSuccess);
    }

    std::shared_ptr<joynr::Future<void>> removeAsync(
            const std::string& participantId,
            std::function<void()> onSuccess = nullptr,
            std::function<void(const joynr::exceptions::JoynrRuntimeException& error)>
                    onRuntimeError = nullptr) override
    {
        return resolve(onSuccess);
    }

private:
    joynr::types::DiscoveryEntry entry;
};

class DummyRequestCallerDirectory : public joynr::IRequestCallerDirectory
{
public:
    virtual std::shared_ptr<RequestCaller> lookupRequestCaller(
            const std::string& participantId) override
    {
        std::ignore = participantId;
        return std::shared_ptr<RequestCaller>();
    }

    virtual bool containsRequestCaller(const std::string& participantId) override
    {
        std::ignore = participantId;
        // By returning false here, we prevent the proxy builder from selecting the in-process
        // connector
        return false;
    }
};

/**
 * @brief Very reduced Runtime which uses DummyDiscovery as the discovery proxy.
 */
class ShortCircuitRuntime : public JoynrRuntime
{
public:
    ShortCircuitRuntime(std::unique_ptr<Settings> settings,
                        std::shared_ptr<IKeychain> keyChain = nullptr);

    template <class TIntfProvider>
    std::string registerProvider(const std::string& domain,
                                 std::shared_ptr<TIntfProvider> provider,
                                 const types::ProviderQos& providerQos)
    {
        return capabilitiesRegistrar->addAsync<TIntfProvider>(
                domain, provider, providerQos, nullptr, nullptr);
    }

    template <class TIntfProvider>
    std::string unregisterProvider(const std::string& domain,
                                   std::shared_ptr<TIntfProvider> provider)
    {
        Future<void> future;
        auto onSuccess = [&future]() { future.onSuccess(); };
        auto onError = [&future](const exceptions::JoynrRuntimeException& exception) {
            future.onError(std::make_shared<exceptions::JoynrRuntimeException>(exception));
        };

        std::string participantId = capabilitiesRegistrar->removeAsync<TIntfProvider>(
                domain, provider, std::move(onSuccess), std::move(onError));
        future.get();
        return participantId;
    }

    template <class TIntfProxy>
    std::shared_ptr<ProxyBuilder<TIntfProxy>> createProxyBuilder(const std::string& domain)
    {
        return std::make_shared<ProxyBuilder<TIntfProxy>>(shared_from_this(),
                                                          *proxyFactory,
                                                          requestCallerDirectory,
                                                          discoveryProxy,
                                                          domain,
                                                          dispatcherAddress,
                                                          messageRouter,
                                                          messagingSettings);
    }

    std::shared_ptr<IMessageRouter> getMessageRouter()
    {
        return messageRouter;
    }

private:
    SingleThreadedIOService singleThreadedIOService;
    std::shared_ptr<IMessageRouter> messageRouter;
    std::shared_ptr<joynr::system::IDiscoveryAsync> discoveryProxy;
    std::shared_ptr<IMessageSender> messageSender;
    std::shared_ptr<IDispatcher> joynrDispatcher;
    std::shared_ptr<IDispatcher> inProcessDispatcher;
    std::shared_ptr<InProcessMessagingSkeleton> dispatcherMessagingSkeleton;
    std::shared_ptr<joynr::system::RoutingTypes::Address> dispatcherAddress;
    std::shared_ptr<PublicationManager> publicationManager;
    std::shared_ptr<SubscriptionManager> subscriptionManager;
    std::shared_ptr<InProcessPublicationSender> inProcessPublicationSender;
    std::unique_ptr<ProxyFactory> proxyFactory;
    std::shared_ptr<ParticipantIdStorage> participantIdStorage;
    std::unique_ptr<CapabilitiesRegistrar> capabilitiesRegistrar;
    std::uint64_t maximumTtlMs;
    std::shared_ptr<IKeychain> keyChain;
    std::shared_ptr<DummyRequestCallerDirectory> requestCallerDirectory;
    ClusterControllerSettings clusterControllerSettings;

    const bool enablePersistency;
};

} // namespace joynr

#endif // SHORTCIRCUITRUNTIME_H
