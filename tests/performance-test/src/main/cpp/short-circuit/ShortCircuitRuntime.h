/*
 * #%L
 * %%
 * Copyright (C) 2016 BMW Car IT GmbH
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

#include "joynr/JoynrRuntime.h"
#include "joynr/IMessageRouter.h"
#include "joynr/JoynrMessageSender.h"
#include "joynr/InProcessPublicationSender.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/SingleThreadedIOService.h"

namespace joynr
{

class InProcessMessagingSkeleton;
class Settings;
class SubscriptionManager;

class DummyDiscovery : public joynr::system::IDiscoverySync
{
public:
    void add(const joynr::types::DiscoveryEntry& discoveryEntry) override
    {
        entry = discoveryEntry;
    }

    void lookup(std::vector<joynr::types::DiscoveryEntry>& result,
                const std::vector<std::string>& domains,
                const std::string& interfaceName,
                const joynr::types::DiscoveryQos& discoveryQos) override
    {
        result.push_back(entry);
    }

    void lookup(

            joynr::types::DiscoveryEntry& result,
            const std::string& participantId) override
    {
        result = entry;
    }

    void remove(const std::string& participantId) override
    {
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
class ShortCircuitRuntime
{
public:
    ShortCircuitRuntime();

    template <class TIntfProvider>
    std::string registerProvider(const std::string& domain,
                                 std::shared_ptr<TIntfProvider> provider,
                                 const types::ProviderQos& providerQos)
    {
        return capabilitiesRegistrar->add<TIntfProvider>(domain, provider, providerQos);
    }

    template <class TIntfProvider>
    std::string unregisterProvider(const std::string& domain,
                                   std::shared_ptr<TIntfProvider> provider)
    {
        return capabilitiesRegistrar->remove<TIntfProvider>(domain, provider);
    }

    template <class TIntfProxy>
    ProxyBuilder<TIntfProxy>* createProxyBuilder(const std::string& domain)
    {
        ProxyBuilder<TIntfProxy>* builder = new ProxyBuilder<TIntfProxy>(*proxyFactory,
                                                                         &requestCallerDirectory,
                                                                         *discoveryProxy,
                                                                         domain,
                                                                         dispatcherAddress,
                                                                         messageRouter,
                                                                         maximumTtlMs);
        return builder;
    }

private:
    SingleThreadedIOService singleThreadedIOService;
    std::shared_ptr<IMessageRouter> messageRouter;
    std::unique_ptr<joynr::system::IDiscoverySync> discoveryProxy;
    std::unique_ptr<JoynrMessageSender> joynrMessageSender;
    IDispatcher* joynrDispatcher;
    IDispatcher* inProcessDispatcher;
    std::shared_ptr<InProcessMessagingSkeleton> dispatcherMessagingSkeleton;
    std::shared_ptr<joynr::system::RoutingTypes::Address> dispatcherAddress;
    PublicationManager* publicationManager;
    SubscriptionManager* subscriptionManager;
    std::unique_ptr<InProcessPublicationSender> inProcessPublicationSender;
    InProcessConnectorFactory* inProcessConnectorFactory;
    JoynrMessagingConnectorFactory* joynrMessagingConnectorFactory;
    std::unique_ptr<ProxyFactory> proxyFactory;
    std::shared_ptr<ParticipantIdStorage> participantIdStorage;
    std::unique_ptr<CapabilitiesRegistrar> capabilitiesRegistrar;
    std::uint64_t maximumTtlMs;
    DummyRequestCallerDirectory requestCallerDirectory;
};

} // namespace joynr

#endif // SHORTCIRCUITRUNTIME_H
