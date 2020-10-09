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
#ifndef JOYNRUNTIME_H
#define JOYNRUNTIME_H

#include <cassert>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "joynr/Future.h"
#include "joynr/JoynrClusterControllerRuntimeExport.h"
#include "joynr/JoynrRuntimeImpl.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Settings.h"
#include "joynr/exceptions/JoynrException.h"

namespace joynr
{

class IKeychain;

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
    ~JoynrRuntime()
    {
        shutdown();
    }

    /**
     * @brief Synchronously shuts down this runtime
     */
    void shutdown()
    {
        _runtimeImpl->shutdown();
    }

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
     * @param awaitGlobalRegistration if set to true, onSuccess will be invoked only after global
     * registration succeeded, respectively onError will be invoked only after global registration
     * failed; default is false (only applicable to global scope)
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
        return _runtimeImpl->registerProviderAsync(domain,
                                                   provider,
                                                   providerQos,
                                                   std::move(onSuccess),
                                                   std::move(onError),
                                                   persist,
                                                   awaitGlobalRegistration,
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
     * @param awaitGlobalRegistration if set to true, method will block until global registration
     * succeeded, respectively it will throw an exception in case global registration failed;
     * default is false (only applicable to global scope)
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
     * @param awaitGlobalRegistration if set to true, onSuccess will be invoked only after global
     * registration succeeded, respectively onError will be invoked only after global registration
     * failed; default is false
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
        return _runtimeImpl->registerProviderInAllBackendsAsync(domain,
                                                                provider,
                                                                providerQos,
                                                                std::move(onSuccess),
                                                                std::move(onError),
                                                                persist,
                                                                awaitGlobalRegistration);
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
     * @param awaitGlobalRegistration if set to true, method will block until global registration
     * succeeded, respectively it will throw an exception in case global registration failed;
     * default is false
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
        return _runtimeImpl->unregisterProviderAsync(
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
        return _runtimeImpl->unregisterProviderAsync(
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
            std::shared_ptr<exceptions::JoynrRuntimeException> exceptionPtr(exception.clone());
            future.onError(std::move(exceptionPtr));
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
        return _runtimeImpl->createProxyBuilder<TIntfProxy>(domain);
    }

    template <class TIntfProxy>
    std::shared_ptr<GuidedProxyBuilder> createGuidedProxyBuilder(const std::string& domain)
    {
        return _runtimeImpl->createGuidedProxyBuilder<TIntfProxy>(domain);
    }

    /**
     * @brief Create a JoynrRuntime object. The call blocks until the runtime is created.
     * @param pathToLibjoynrSettings
     * @param onFatalRuntimeError Called in case a runtime error prevents further communication
     * @param pathToMessagingSettings
     * @param An optional key chain that is used for websocket connections
     * @return pointer to a JoynrRuntime instance
     */
    static std::shared_ptr<JoynrRuntime> createRuntime(
            const std::string& pathToLibjoynrSettings,
            std::function<void(const exceptions::JoynrRuntimeException&)> onFatalRuntimeError,
            const std::string& pathToMessagingSettings = "",
            std::shared_ptr<IKeychain> keyChain = nullptr);

    /**
     * @brief Create a JoynrRuntime object. The call blocks until the runtime is created.
     * @param pathToLibjoynrSettings
     * @param pathToMessagingSettings
     * @param An optional key chain that is used for websocket connections
     * @return pointer to a JoynrRuntime instance
     *
     * [[deprecated("Use createRuntime(const std::string&, std::function<void(const "
     *             "joynr::exceptions::JoynrRuntimeException&)>, const std::string&, "
     *             "std::shared_ptr<IKeychain>) instead.")]]
     */
    inline static std::shared_ptr<JoynrRuntime> createRuntime(
            const std::string& pathToLibjoynrSettings,
            const std::string& pathToMessagingSettings = "",
            std::shared_ptr<IKeychain> keyChain = nullptr)
    {
        return createRuntime(pathToLibjoynrSettings,
                             defaultFatalRuntimeErrorHandler,
                             pathToMessagingSettings,
                             std::move(keyChain));
    }

    /**
     * @brief Create a JoynrRuntime object. The call blocks until the runtime is created.
     * @param settings settings object
     * @param onFatalRuntimeError Called in case a runtime error prevents further communication
     * @param An optional key chain that is used for websocket connections
     * @return pointer to a JoynrRuntime instance
     */
    static std::shared_ptr<JoynrRuntime> createRuntime(
            std::unique_ptr<Settings> settings,
            std::function<void(const exceptions::JoynrRuntimeException&)> onFatalRuntimeError,
            std::shared_ptr<IKeychain> keyChain = nullptr);

    /**
     * @brief Create a JoynrRuntime object. The call blocks until the runtime is created.
     * @param settings settings object
     * @param An optional key chain that is used for websocket connections
     * @return pointer to a JoynrRuntime instance
     *
     * [[deprecated("Use createRuntime(std::unique_ptr<Settings>, std::function<void(const "
     *             "joynr::exceptions::JoynrRuntimeException&)>, std::shared_ptr<IKeychain>) "
     *             "instead.")]]
     */
    inline static std::shared_ptr<JoynrRuntime> createRuntime(
            std::unique_ptr<Settings> settings,
            std::shared_ptr<IKeychain> keyChain = nullptr)
    {
        return createRuntime(
                std::move(settings), defaultFatalRuntimeErrorHandler, std::move(keyChain));
    }

    /**
     * @brief Create a JoynrRuntime object asynchronously. The call does not block. A callback
     * will be called when the runtime creation finished.
     * @param pathToLibjoynrSettings Path to lib joynr setting files
     * @param onFatalRuntimeError Called (after successful setup) in case a runtime error prevents
     * further communication
     * @param onSuccess Is called when the runtime is available for use
     * @param onError Is called when an error occurs during runtime setup
     * @param pathToMessagingSettings
     * @param An optional key chain that is used for websocket connections
     * @return shared_ptr to the JoynrRuntime instance; this instance MUST NOT be used before
     * onSuccess is called
     */
    static std::shared_ptr<JoynrRuntime> createRuntimeAsync(
            const std::string& pathToLibjoynrSettings,
            std::function<void(const exceptions::JoynrRuntimeException&)> onFatalRuntimeError,
            std::function<void()> onSuccess,
            std::function<void(const exceptions::JoynrRuntimeException& exception)> onError,
            const std::string& pathToMessagingSettings = "",
            std::shared_ptr<IKeychain> keyChain = nullptr) noexcept;

    /**
     * @brief Create a JoynrRuntime object asynchronously. The call does not block. A callback
     * will be called when the runtime creation finished.
     * @param pathToLibjoynrSettings Path to lib joynr setting files
     * @param onSuccess Is called when the runtime is available for use
     * @param onError Is called when an error occurs during runtime setup
     * @param pathToMessagingSettings
     * @param An optional key chain that is used for websocket connections
     * @return shared_ptr to the JoynrRuntime instance; this instance MUST NOT be used before
     * onSuccess is called
     *
     * [[deprecated("Use createRuntimeAsync(const std::string&, std::function<void(const "
     *             "joynr::exceptions::JoynrRuntimeException&)>, std::function<void()>, "
     *             "std::function<void(const exceptions::JoynrRuntimeException&)>, const "
     *             "std::string&, std::shared_ptr<IKeychain>) instead.")]]
     */
    inline static std::shared_ptr<JoynrRuntime> createRuntimeAsync(
            const std::string& pathToLibjoynrSettings,
            std::function<void()> onSuccess,
            std::function<void(const exceptions::JoynrRuntimeException& exception)> onError,
            const std::string& pathToMessagingSettings = "",
            std::shared_ptr<IKeychain> keyChain = nullptr) noexcept
    {
        return createRuntimeAsync(pathToLibjoynrSettings,
                                  defaultFatalRuntimeErrorHandler,
                                  std::move(onSuccess),
                                  std::move(onError),
                                  pathToMessagingSettings,
                                  std::move(keyChain));
    }

    /**
     * @brief Create a JoynrRuntime object asynchronously. The call does not block. A callback
     * will be called when the runtime creation finished.
     * @param settings settings object
     * @param onFatalRuntimeError Called (after successful setup) in case a runtime error prevents
     * further communication
     * @param onSuccess Is called when the runtime is available for use
     * @param onError Is called when an error occurs during runtime setup
     * @param An optional key chain that is used for websocket connections
     * @return shared_ptr to the JoynrRuntime instance; this instance MUST NOT be used before
     * onSuccess is called
     */
    static std::shared_ptr<JoynrRuntime> createRuntimeAsync(
            std::unique_ptr<Settings> settings,
            std::function<void(const exceptions::JoynrRuntimeException&)> onFatalRuntimeError,
            std::function<void()> onSuccess,
            std::function<void(const exceptions::JoynrRuntimeException& exception)> onError,
            std::shared_ptr<IKeychain> keyChain = nullptr) noexcept;

    /**
     * @brief Create a JoynrRuntime object asynchronously. The call does not block. A callback
     * will be called when the runtime creation finished.
     * @param settings settings object
     * @param onSuccess Is called when the runtime is available for use
     * @param onError Is called when an error occurs during runtime setup
     * @param An optional key chain that is used for websocket connections
     * @return shared_ptr to the JoynrRuntime instance; this instance MUST NOT be used before
     * onSuccess is called
     *
     * [[deprecated("Use createRuntimeAsync(std::unique_ptr<Settings>, std::function<void(const "
     *             "joynr::exceptions::JoynrRuntimeException&)>, std::function<void()>, "
     *             "std::function<void(const exceptions::JoynrRuntimeException&)>, "
     *            "std::shared_ptr<IKeychain>) instead.")]]
     */
    inline static std::shared_ptr<JoynrRuntime> createRuntimeAsync(
            std::unique_ptr<Settings> settings,
            std::function<void()> onSuccess,
            std::function<void(const exceptions::JoynrRuntimeException& exception)> onError,
            std::shared_ptr<IKeychain> keyChain = nullptr) noexcept
    {
        return createRuntimeAsync(std::move(settings),
                                  defaultFatalRuntimeErrorHandler,
                                  std::move(onSuccess),
                                  std::move(onError),
                                  std::move(keyChain));
    }

    /**
     * @brief Constructs a JoynrRuntime instance
     * @param settings The system service settings
     */
    explicit JoynrRuntime(std::shared_ptr<JoynrRuntimeImpl> runtimeImpl)
            : _runtimeImpl(std::move(runtimeImpl))
    {
    }

    inline static void defaultFatalRuntimeErrorHandler(
            const exceptions::JoynrRuntimeException& error)
    {
        JOYNR_LOG_FATAL(
                JoynrRuntime::logger(), "Unexpected JOYNR runtime error occured: {}", error.what());
    }

protected:
private:
    std::shared_ptr<JoynrRuntimeImpl> _runtimeImpl;
    DISALLOW_COPY_AND_ASSIGN(JoynrRuntime);
    ADD_LOGGER(JoynrRuntime)
};

} // namespace joynr
#endif // JOYNRUNTIME_H
