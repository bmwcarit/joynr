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
#ifndef JOYNRUNTIMEIMPL_H
#define JOYNRUNTIMEIMPL_H

#include <cassert>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "joynr/CapabilitiesRegistrar.h"
#include "joynr/Future.h"
#include "joynr/GuidedProxyBuilder.h"
#include "joynr/JoynrClusterControllerRuntimeExport.h"
#include "joynr/LocalDiscoveryAggregator.h"
#include "joynr/MessagingSettings.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/ProxyBuilder.h"
#include "joynr/SystemServicesSettings.h"
#include "joynr/exceptions/JoynrException.h"

namespace joynr
{

class IKeychain;
class IMessageRouter;
class IRequestCallerDirectory;
class ParticipantIdStorage;
class ProxyFactory;
class PublicationManager;
class Settings;
class SingleThreadedIOService;

struct Logger;

namespace system
{
namespace RoutingTypes
{
class Address;
}
}

namespace types
{
class DiscoveryEntryWithMetaInfo;
class ProviderQos;
}

/**
 * @brief Class representing the central Joynr Api object,
 * used to register / unregister providers and create proxy builders
 */
class JOYNRCLUSTERCONTROLLERRUNTIME_EXPORT JoynrRuntimeImpl
        : public std::enable_shared_from_this<JoynrRuntimeImpl>
{
public:
    /**
     * @brief Destroys a JoynrRuntime instance
     */
    virtual ~JoynrRuntimeImpl();
    virtual void shutdown();

    /**
     * @brief Registers a provider with the joynr communication framework asynchronously.
     *
     * If registration to local and global scope is requested by 'providerQos' parameter,
     * the provider is registered to all GBIDs configured in the cluster controller.
     *
     * The 'gbids' parameter can be provided to override the GBIDs selection in the cluster
     * controller. The global capabilities directory identified by the first selected GBID performs
     * the registration.
     *
     * @tparam TIntfProvider The interface class of the provider to register. The corresponding
     * template parameter of a Franca interface called "MyDemoIntf" is "MyDemoIntfProvider".
     * @param domain The domain to register the provider on. Has to be
     * identical at the client to be able to find the provider.
     * @param provider The provider instance to register.
     * @param providerQos The qos associated with the registered provider.
     * @param onSucess: Will be invoked when provider registration succeeded.
     * @param onError: Will be invoked when the provider could not be registered. An exception,
     * which describes the error, is passed as the parameter.
     * @param persist if set to true, participant ID of the provider will be persisted,
     * otherwise it will not; default is true
     * @param awaitGlobalRegistration: true if global registration should be waited for
     * (only applicable to global scope)
     * @param gbids: Optional subset of GBIDs configured in the cluster controller for custom global
     * registration (only applicable to global scope)
     * @return The globally unique participant ID of the provider. It is assigned by the joynr
     * communication framework.
     */
    template <class TIntfProvider>
    std::string registerProviderAsync(
            const std::string& domain,
            std::shared_ptr<TIntfProvider> provider,
            const joynr::types::ProviderQos& providerQos,
            std::function<void()> onSuccess,
            std::function<void(const exceptions::JoynrRuntimeException&)> onError,
            bool persist = true,
            bool awaitGlobalRegistration = false,
            std::vector<std::string> gbids = std::vector<std::string>()) noexcept
    {
        assert(_capabilitiesRegistrar);
        assert(!domain.empty());
        /*
         * Though the description states that for an empty GBID list, the
         * registration should be applied to all, the 'addToAll' is set to false.
         * Instead on the CC side, the 'add' processing is taking care that registration
         * is applied to all known backends.
         */
        const bool addToAll = false;
        return _capabilitiesRegistrar->addAsync(domain,
                                                provider,
                                                providerQos,
                                                std::move(onSuccess),
                                                std::move(onError),
                                                persist,
                                                awaitGlobalRegistration,
                                                addToAll,
                                                gbids);
    }

    /**
     * @brief Registers a provider with the joynr communication framework.
     *
     * If registration to local and global scope is requested by 'providerQos' parameter,
     * the provider is registered to all GBIDs configured in the cluster controller.
     *
     * The 'gbids' parameter can be provided to override the GBIDs selection in the cluster
     * controller. The global capabilities directory identified by the first selected GBID performs
     * the registration.
     *
     * @tparam TIntfProvider The interface class of the provider to register. The corresponding
     * template parameter of a Franca interface called "MyDemoIntf" is "MyDemoIntfProvider".
     * @param domain The domain to register the provider on. Has to be
     * identical at the client to be able to find the provider.
     * @param provider The provider instance to register.
     * @param providerQos The qos associated with the registered provider.
     * @param persist if set to true, participant ID of the provider will be persisted,
     * otherwise it will not; default is true
     * @param awaitGlobalRegistration: true if global registration should be waited for
     * (only applicable to global scope)
     * @param gbids: Optional subset of GBIDs configured in the cluster controller for custom global
     * registration (only applicable to global scope)
     * @return The globally unique participant ID of the provider. It is assigned by the joynr
     * communication framework.
     */
    template <class TIntfProvider>
    std::string registerProvider(const std::string& domain,
                                 std::shared_ptr<TIntfProvider> provider,
                                 const joynr::types::ProviderQos& providerQos,
                                 bool persist = true,
                                 bool awaitGlobalRegistration = false,
                                 std::vector<std::string> gbids = std::vector<std::string>())
    {
        Future<void> future;
        auto onSuccess = [&future]() { future.onSuccess(); };
        auto onError = [&future](const exceptions::JoynrRuntimeException& exception) {
            future.onError(std::make_shared<exceptions::JoynrRuntimeException>(exception));
        };

        std::string participiantId = registerProviderAsync(domain,
                                                           provider,
                                                           providerQos,
                                                           std::move(onSuccess),
                                                           std::move(onError),
                                                           persist,
                                                           awaitGlobalRegistration,
                                                           gbids);
        future.get();
        return participiantId;
    }

    /**
     * @brief Registers a provider with the joynr communication framework asynchronously
     * in all backends known to the cluster controller (in case of global registration).
     * @tparam TIntfProvider The interface class of the provider to register. The corresponding
     * template parameter of a Franca interface called "MyDemoIntf" is "MyDemoIntfProvider".
     * @param domain The domain to register the provider on. Has to be
     * identical at the client to be able to find the provider.
     * @param provider The provider instance to register.
     * @param providerQos The qos associated with the registered provider.
     * @param onSucess: Will be invoked when provider registration succeeded.
     * @param onError: Will be invoked when the provider could not be registered. An exception,
     * which describes the error, is passed as the parameter.
     * @param persist if set to true, participant ID of the provider will be persisted,
     * otherwise it will not; default is true
     * @param awaitGlobalRegistration: true if global registration should be waited for
     * @return The globally unique participant ID of the provider. It is assigned by the joynr
     * communication framework.
     *
     * [[deprecated("Use registerProviderAsync(const std::string&, std::shared_ptr<TIntfProvider>,
     *                                         const joynr::types::ProviderQos&,
     *                                         std::function<void()>, std::function<void(const
     *                                         exceptions::JoynrRuntimeException&)>, bool, bool,
     *                                         std::vector<std::string>) instead.")]]
     */
    template <class TIntfProvider>
    std::string registerProviderInAllBackendsAsync(
            const std::string& domain,
            std::shared_ptr<TIntfProvider> provider,
            const joynr::types::ProviderQos& providerQos,
            std::function<void()> onSuccess,
            std::function<void(const exceptions::JoynrRuntimeException&)> onError,
            bool persist = true,
            bool awaitGlobalRegistration = false) noexcept
    {
        assert(_capabilitiesRegistrar);
        assert(!domain.empty());
        const bool addToAll = true;
        return _capabilitiesRegistrar->addAsync(domain,
                                                provider,
                                                providerQos,
                                                std::move(onSuccess),
                                                std::move(onError),
                                                persist,
                                                awaitGlobalRegistration,
                                                addToAll);
    }

    /**
     * @brief Registers a provider with the joynr communication framework
     * in all backends known to the cluster controller (in case of global registration).
     * @tparam TIntfProvider The interface class of the provider to register. The corresponding
     * template parameter of a Franca interface called "MyDemoIntf" is "MyDemoIntfProvider".
     * @param domain The domain to register the provider on. Has to be
     * identical at the client to be able to find the provider.
     * @param provider The provider instance to register.
     * @param providerQos The qos associated with the registered provider.
     * @param persist if set to true, participant ID of the provider will be persisted,
     * otherwise it will not; default is true
     * @param awaitGlobalRegistration: true if global registration should be waited for
     * @return The globally unique participant ID of the provider. It is assigned by the joynr
     * communication framework.
     *
     * [[deprecated("Use registerProvider(const std::string&, std::shared_ptr<TIntfProvider>,
     *                                    const joynr::types::ProviderQos&, bool, bool,
     *                                    std::vector<std::string>) instead.")]]
     */
    template <class TIntfProvider>
    std::string registerProviderInAllBackends(const std::string& domain,
                                              std::shared_ptr<TIntfProvider> provider,
                                              const joynr::types::ProviderQos& providerQos,
                                              bool persist = true,
                                              bool awaitGlobalRegistration = false)
    {
        Future<void> future;
        auto onSuccess = [&future]() { future.onSuccess(); };
        auto onError = [&future](const exceptions::JoynrRuntimeException& exception) {
            future.onError(std::make_shared<exceptions::JoynrRuntimeException>(exception));
        };

        std::string participiantId = registerProviderInAllBackendsAsync(domain,
                                                                        provider,
                                                                        providerQos,
                                                                        std::move(onSuccess),
                                                                        std::move(onError),
                                                                        persist,
                                                                        awaitGlobalRegistration);
        future.get();
        return participiantId;
    }

    /**
     * @brief Unregisters the provider from the joynr communication framework.
     *
     * Unregister a provider identified by its globally unique participant ID. The participant ID is
     * returned during the provider registration process.
     * @param participantId The participantId of the provider which shall be unregistered
     * @param onSucess: Will be invoked when provider unregistration succeeded.
     * @param onError: Will be invoked when the provider could not be unregistered. An exception,
     * which describes the error, is passed as the parameter.
     */
    void unregisterProviderAsync(
            const std::string& participantId,
            std::function<void()> onSuccess,
            std::function<void(const exceptions::JoynrRuntimeException&)> onError) noexcept
    {
        assert(_capabilitiesRegistrar);
        _capabilitiesRegistrar->removeAsync(
                participantId, std::move(onSuccess), std::move(onError));
    }

    /**
     * @brief Unregisters the provider from the joynr framework
     * @tparam TIntfProvider The interface class of the provider to unregister. The corresponding
     * template parameter of a Franca interface called "MyDemoIntf" is "MyDemoIntfProvider".
     * @param domain The domain to unregister the provider from. It must match the domain used
     * during provider registration.
     * @param provider The provider instance to unregister the provider from.
     * @param onSucess: Will be invoked when provider unregistration succeeded.
     * @param onError: Will be invoked when the provider could not be unregistered. An exception,
     * which describes the error, is passed as the parameter.
     * @return The globally unique participant ID of the provider. It is assigned by the joynr
     * communication framework.
     */
    template <class TIntfProvider>
    std::string unregisterProviderAsync(
            const std::string& domain,
            std::shared_ptr<TIntfProvider> provider,
            std::function<void()> onSuccess,
            std::function<void(const exceptions::JoynrRuntimeException&)> onError) noexcept
    {
        assert(_capabilitiesRegistrar);
        assert(!domain.empty());
        return _capabilitiesRegistrar->removeAsync(
                domain, provider, std::move(onSuccess), std::move(onError));
    }

    /**
     * @brief Unregisters the provider from the joynr communication framework.
     *
     * Unregister a provider identified by its globally unique participant ID. The participant ID is
     * returned during the provider registration process.
     * @param participantId The participantId of the provider which shall be unregistered
     */
    void unregisterProvider(const std::string& participantId)
    {
        Future<void> future;
        auto onSuccess = [&future]() { future.onSuccess(); };
        auto onError = [&future](const exceptions::JoynrRuntimeException& exception) {
            future.onError(std::make_shared<exceptions::JoynrRuntimeException>(exception));
        };

        unregisterProviderAsync(participantId, std::move(onSuccess), std::move(onError));
        future.get();
    }

    /**
     * @brief Unregisters the provider from the joynr framework
     * @tparam TIntfProvider The interface class of the provider to unregister. The corresponding
     * template parameter of a Franca interface called "MyDemoIntf" is "MyDemoIntfProvider".
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
        assert(!domain.empty());
        Future<void> future;
        auto onSuccess = [&future]() { future.onSuccess(); };
        auto onError = [&future](const exceptions::JoynrRuntimeException& exception) {
            future.onError(std::make_shared<exceptions::JoynrRuntimeException>(exception));
        };
        std::string participantId =
                unregisterProviderAsync(domain, provider, std::move(onSuccess), std::move(onError));
        future.get();
        return participantId;
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
    std::shared_ptr<ProxyBuilder<TIntfProxy>> createProxyBuilder(const std::string& domain)
    {
        if (!_proxyFactory) {
            throw exceptions::JoynrRuntimeException(
                    "Exception in JoynrRuntime: Cannot perform arbitration as "
                    "runtime is not yet fully initialized.");
        }

        auto proxyBuilder = std::make_shared<ProxyBuilder<TIntfProxy>>(shared_from_this(),
                                                                       *_proxyFactory,
                                                                       _requestCallerDirectory,
                                                                       _discoveryProxy,
                                                                       domain,
                                                                       _dispatcherAddress,
                                                                       getMessageRouter(),
                                                                       _messagingSettings);
        std::lock_guard<std::mutex> lock(_proxyBuildersMutex);
        _proxyBuilders.push_back(proxyBuilder);
        return proxyBuilder;
    }

    template <class TIntfProxy>
    std::shared_ptr<GuidedProxyBuilder> createGuidedProxyBuilder(const std::string& domain)
    {
        if (!_proxyFactory) {
            throw exceptions::JoynrRuntimeException(
                    "Exception in JoynrRuntime: Cannot perform arbitration as "
                    "runtime is not yet fully initialized.");
        }

        std::string interfaceName = TIntfProxy::INTERFACE_NAME();
        auto guidedProxyBuilder = std::make_shared<GuidedProxyBuilder>(shared_from_this(),
                                                                       *_proxyFactory,
                                                                       _requestCallerDirectory,
                                                                       _discoveryProxy,
                                                                       domain,
                                                                       _dispatcherAddress,
                                                                       getMessageRouter(),
                                                                       _messagingSettings,
                                                                       interfaceName);

        std::lock_guard<std::mutex> lock(_proxyBuildersMutex);
        _proxyBuilders.push_back(guidedProxyBuilder);
        return guidedProxyBuilder;
    }

    static std::unique_ptr<Settings> createSettings(const std::string& pathToLibjoynrSettings,
                                                    const std::string& pathToMessagingSettings);

protected:
    // NOTE: The implementation of the constructor and destructor must be inside this
    // header file because there are multiple implementations (cpp files) in folder
    // cluster-controller-runtime and libjoynr-runtime.

    /**
     * @brief Constructs a JoynrRuntime instance
     * @param settings The system service settings
     * @param onFatalRuntimeError Called in case a runtime error prevents further communication
     */
    explicit JoynrRuntimeImpl(
            Settings& settings,
            std::function<void(const exceptions::JoynrRuntimeException&)>&& onFatalRuntimeError,
            std::shared_ptr<IKeychain> keyChain = nullptr);

    /** @brief Return an IMessageRouter instance */
    virtual std::shared_ptr<IMessageRouter> getMessageRouter() = 0;

    bool checkAndLogCryptoFileExistence(const std::string& caPemFile,
                                        const std::string& certPemFile,
                                        const std::string& privateKeyPemFile,
                                        Logger& logger);

    /** @brief Get provisioned entries.
     *  @return A map participantId -> DiscoveryEntryWithMetaInfo.
     */
    virtual std::map<std::string, joynr::types::DiscoveryEntryWithMetaInfo> getProvisionedEntries()
            const;

    std::shared_ptr<SingleThreadedIOService> _singleThreadIOService;

    /** @brief Factory for creating proxy instances */
    std::unique_ptr<ProxyFactory> _proxyFactory;
    /** Is forwarded to proxy builder objects. They use it to identify in-process providers **/
    std::shared_ptr<IRequestCallerDirectory> _requestCallerDirectory;
    /** @brief Creates and persists participant id */
    std::shared_ptr<ParticipantIdStorage> _participantIdStorage;
    /** @brief Class that handles provider registration/deregistration */
    std::unique_ptr<CapabilitiesRegistrar> _capabilitiesRegistrar;
    /**
     * @brief Messaging settings
     */
    MessagingSettings _messagingSettings;
    /** @brief System services settings */
    SystemServicesSettings _systemServicesSettings;
    /** @brief User callback invoked in case an error occurs which prevents further communication */
    std::function<void(const exceptions::JoynrRuntimeException&)> _onFatalRuntimeError;
    /** @brief Address of the dispatcher */
    std::shared_ptr<const joynr::system::RoutingTypes::Address> _dispatcherAddress;
    /** @brief Wrapper for discovery proxies */
    std::shared_ptr<LocalDiscoveryAggregator> _discoveryProxy;
    /**
     * @brief Publication manager receives subscription requests and prepares publications
     * which are send back to the subscription manager.
     */
    std::shared_ptr<PublicationManager> _publicationManager;
    std::shared_ptr<IKeychain> _keyChain;
    std::vector<std::shared_ptr<IProxyBuilderBase>> _proxyBuilders;
    std::mutex _proxyBuildersMutex;

private:
    DISALLOW_COPY_AND_ASSIGN(JoynrRuntimeImpl);
};

} // namespace joynr
#endif // JOYNRUNTIMEIMPL_H
