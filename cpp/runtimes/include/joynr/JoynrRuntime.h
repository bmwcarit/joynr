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
#ifndef JOYNRUNTIME_H
#define JOYNRUNTIME_H

#include <cassert>
#include <string>
#include <memory>
#include <functional>

#include "joynr/PrivateCopyAssign.h"
#include "joynr/JoynrClusterControllerRuntimeExport.h"
#include "joynr/CapabilitiesRegistrar.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/ProxyBuilder.h"
#include "joynr/ParticipantIdStorage.h"
#include "joynr/ProxyFactory.h"
#include "joynr/MessagingSettings.h"
#include "joynr/SystemServicesSettings.h"
#include "joynr/system/DiscoveryProxy.h"
#include "joynr/LocalDiscoveryAggregator.h"
#include "joynr/PublicationManager.h"

namespace joynr
{

class SingleThreadedIOService;
/**
 * @brief Class representing the central Joynr Api object,
 * used to register / unregister providers and create proxy builders
 */
class JOYNRCLUSTERCONTROLLERRUNTIME_EXPORT JoynrRuntime
{
public:
    /**
     * @brief Destroys a JoynrRuntime instance
     */
    virtual ~JoynrRuntime();

    /**
     * @brief Registers a provider with the joynr communication framework.
     * @tparam TIntfProvider The interface class of the provider to register. The corresponding
     * template parameter of a Franca interface called "MyDemoIntf" is "MyDemoIntfProvider".
     * @param domain The domain to register the provider on. Has to be
     * identical at the client to be able to find the provider.
     * @param provider The provider instance to register.
     * @param providerQos The qos associated with the registered provider.
     * @return The globally unique participant ID of the provider. It is assigned by the joynr
     * communication framework.
     */
    template <class TIntfProvider>
    std::string registerProvider(const std::string& domain,
                                 std::shared_ptr<TIntfProvider> provider,
                                 const joynr::types::ProviderQos& providerQos)
    {
        assert(capabilitiesRegistrar);
        assert(!domain.empty());
        return capabilitiesRegistrar->add<TIntfProvider>(domain, provider, providerQos);
    }

    /**
     * @brief Unregisters the provider from the joynr communication framework.
     *
     * Unregister a provider identified by its globally unique participant ID. The participant ID is
     * returned during the provider registration process.
     * @param participantId The participantId of the provider which shall be unregistered
     */
    virtual void unregisterProvider(const std::string& participantId) = 0;

    /**
     * @brief Unregisters the provider from the joynr framework
     * @tparam TIntfProvider The interface class of the provider to unregister. The corresponding
     * template parameter of a Franca interface called "MyDemoIntf" is "MyDemoIntfProvider".
     * @param domain The domain the provider was registered for.
     * @param domain The domain to unregister the provider from. It must match the domain used
     * during provider registration.
     * @param provider The provider instance to unregister the provider from.
     * @return The globally unique participant ID of the provider. It is assigned by the joynr
     * communication framework.
     */
    template <class TIntfProvider>
    std::string unregisterProvider(const std::string& domain,
                                   std::shared_ptr<TIntfProvider> provider)
    {
        assert(capabilitiesRegistrar);
        assert(!domain.empty());
        return capabilitiesRegistrar->remove<TIntfProvider>(domain, provider);
    }

    /**
     * @brief Creates a new proxy builder for the given domain and interface.
     *
     * The proxy builder is used to create a proxy object for a remote provider. It is already
     * bound to a domain and communication interface as defined in Franca. After configuration is
     * finished, ProxyBuilder::build() is called to create the proxy object.
     *
     * @tparam TIntfProxy The interface class of the proxy to create. The corresponding template
     * parameter of a Franca interface called "MyDemoIntf" is "MyDemoIntfProxy".
     * @param domain The domain to connect this proxy to.
     * @return Pointer to the proxybuilder<T> instance
     * @return A proxy builder object that can be used to create proxies.
     */
    template <class TIntfProxy>
    std::unique_ptr<ProxyBuilder<TIntfProxy>> createProxyBuilder(const std::string& domain)
    {
        if (!proxyFactory) {
            throw exceptions::JoynrRuntimeException(
                    "Exception in JoynrRuntime: Creating a proxy before "
                    "startMessaging was called is not yet supported.");
        }
        return std::make_unique<ProxyBuilder<TIntfProxy>>(*proxyFactory,
                                                          requestCallerDirectory,
                                                          *discoveryProxy,
                                                          domain,
                                                          dispatcherAddress,
                                                          messageRouter,
                                                          messagingSettings.getMaximumTtlMs());
    }

    /**
     * @brief Create a JoynrRuntime object. The call blocks until the runtime is created.
     * @param pathToLibjoynrSettings
     * @param pathToMessagingSettings
     * @return pointer to a JoynrRuntime instance
     */
    static std::unique_ptr<JoynrRuntime> createRuntime(
            const std::string& pathToLibjoynrSettings,
            const std::string& pathToMessagingSettings = "");

    /**
     * @brief Create a JoynrRuntime object. The call blocks until the runtime is created.
     * @param settings settings object
     * @return pointer to a JoynrRuntime instance
     */
    static std::unique_ptr<JoynrRuntime> createRuntime(std::unique_ptr<Settings> settings);

    /**
     * @brief Create a JoynrRuntime object. The call does not block. A callback
     * will be called when the runtime creation finished.
     * @param pathToLibjoynrSettings Path to lib joynr setting files
     * @param runtimeCreatedCallback Is called when the runtime is available
     * @param runtimeCreationErrorCallback Is called when an error occurs
     * @param pathToMessagingSettings
     * @return pointer to a JoynrRuntime instance
     */
    static void createRuntimeAsync(
            const std::string& pathToLibjoynrSettings,
            std::function<void(std::unique_ptr<JoynrRuntime> createdRuntime)>
                    runtimeCreatedCallback,
            std::function<void(const exceptions::JoynrRuntimeException& exception)>
                    runtimeCreationErrorCallback,
            const std::string& pathToMessagingSettings = "");

    /**
     * @brief Create a JoynrRuntime object. The call does not block. A callback
     * will be called when the runtime creation finished.
     * @param settings settings object
     * @param runtimeCreatedCallback Is called when the runtime is available
     * @param runtimeCreationErrorCallback Is called when an error occurs
     * @return pointer to a JoynrRuntime instance
     */
    static void createRuntimeAsync(
            std::unique_ptr<Settings> settings,
            std::function<void(std::unique_ptr<JoynrRuntime> createdRuntime)>
                    runtimeCreatedCallback,
            std::function<void(const exceptions::JoynrRuntimeException& exception)>
                    runtimeCreationErrorCallback);

protected:
    // NOTE: The implementation of the constructor and destructor must be inside this
    // header file because there are multiple implementations (cpp files) in folder
    // cluster-controller-runtime and libjoynr-runtime.

    /**
     * @brief Constructs a JoynrRuntime instance
     * @param settings The system service settings
     */
    explicit JoynrRuntime(Settings& settings);

    static std::unique_ptr<Settings> createSettings(const std::string& pathToLibjoynrSettings,
                                                    const std::string& pathToMessagingSettings);

    bool checkAndLogCryptoFileExistence(const std::string& caPemFile,
                                        const std::string& certPemFile,
                                        const std::string& privateKeyPemFile,
                                        Logger& logger);

    std::unique_ptr<SingleThreadedIOService> singleThreadIOService;

    /** @brief Factory for creating proxy instances */
    std::unique_ptr<ProxyFactory> proxyFactory;
    /** Is forwarded to proxy builder objects. They use it to identify in-process providers **/
    IRequestCallerDirectory* requestCallerDirectory;
    /** @brief Creates and persists participant id */
    std::shared_ptr<ParticipantIdStorage> participantIdStorage;
    /** @brief Class that handles provider registration/deregistration */
    std::unique_ptr<CapabilitiesRegistrar> capabilitiesRegistrar;
    /**
     * @brief Messaging settings
     */
    MessagingSettings messagingSettings;
    /** @brief System services settings */
    SystemServicesSettings systemServicesSettings;
    /** @brief Address of the dispatcher */
    std::shared_ptr<const joynr::system::RoutingTypes::Address> dispatcherAddress;
    /** @brief MessageRouter instance */
    std::shared_ptr<MessageRouter> messageRouter;
    /** @brief Wrapper for discovery proxies */
    std::unique_ptr<LocalDiscoveryAggregator> discoveryProxy;
    /**
     * @brief Publication manager receives subscription requests and prepares publications
     * which are send back to the subscription manager.
     */
    PublicationManager* publicationManager;

private:
    DISALLOW_COPY_AND_ASSIGN(JoynrRuntime);
};

} // namespace joynr
#endif // JOYNRUNTIME_H
