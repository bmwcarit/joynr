/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

#include "joynr/PrivateCopyAssign.h"

#include "joynr/JoynrClusterControllerRuntimeExport.h"

#include "joynr/CapabilitiesRegistrar.h"
#include "joynr/exceptions.h"
#include "joynr/ProxyBuilder.h"
#include "joynr/ParticipantIdStorage.h"
#include "joynr/ProxyFactory.h"
#include "joynr/SystemServicesSettings.h"
#include "joynr/system/DiscoveryProxy.h"
#include "joynr/LocalDiscoveryAggregator.h"
#include "joynr/PublicationManager.h"
#include "joynr/IBroadcastFilter.h"
#include "joynr/TypeUtil.h"

#include <QString>
#include <QSharedPointer>
#include <cassert>
#include <memory>
#include <string>

namespace joynr
{

/*!
 * \brief The JoynrRuntime class is the entry point to the joynr communication framework. It offers
 *        functionality to register providers and create corresponding proxy objects.
 */
class JOYNRCLUSTERCONTROLLERRUNTIME_EXPORT JoynrRuntime
{

public:
    // NOTE: The implementation of the constructor and destructor must be inside this
    // header file because there are multiple implementations (cpp files) in folder
    // cluster-controller-runtime and libjoynr-runtime.
    JoynrRuntime(QSettings& settings)
            : proxyFactory(NULL),
              participantIdStorage(NULL),
              capabilitiesRegistrar(NULL),
              systemServicesSettings(settings),
              dispatcherAddress(NULL),
              messageRouter(NULL),
              discoveryProxy(NULL),
              publicationManager(NULL)
    {
        systemServicesSettings.printSettings();
    }

    virtual ~JoynrRuntime()
    {
        delete discoveryProxy;
    }

    /*!
     * \brief Registers a provider with the joynr communication framework.
     *
     * \tparam TIntfProvider The interface class of the provider to register. The corresponding
     *                       template parameter of a Franca interface called "MyDemoIntf" is
     *                       "MyDemoIntfProvider".
     * \param domain The domain to register the provider on.
     * \param provider The provider instance to register.
     * \return The globaly unique participant ID of the provider. It is assigned by the joynr
     *         communication framework.
     */
    template <class TIntfProvider>
    std::string registerProvider(const std::string& domain, std::shared_ptr<TIntfProvider> provider)
    {
        assert(capabilitiesRegistrar);
        assert(!domain.empty());
        return TypeUtil::convertQStringtoStdString(capabilitiesRegistrar->add<TIntfProvider>(
                TypeUtil::convertStdStringtoQString(domain), provider));
    }

    /*!
     * \brief Unregister a provider from the joynr communication framework.
     *
     * Unregister a provider identified by its globaly unique participant ID. The participant ID is
     * returned during the provider registration process.
     *
     * \param participantId The participant ID of the provider to unregister.
     */
    virtual void unregisterProvider(const std::string& participantId) = 0;

    /*!
     * \brief Unregister a provider from the joynr communication framework.
     *
     * \tparam TIntfProvider The interface class of the provider to unregister. The corresponding
     *                       template parameter of a Franca interface called "MyDemoIntf" is
     *                       "MyDemoIntfProvider".
     * \param domain The domain to unregister the provider from. It must match the domain used
     *               during provider registration.
     * \param provider The provider instance to unregister the provider from.
     * \return The globaly unique participant ID of the provider. It is assigned by the joynr
     *         communication framework.
     */
    template <class TIntfProvider>
    std::string unregisterProvider(const std::string& domain,
                                   std::shared_ptr<TIntfProvider> provider)
    {
        assert(capabilitiesRegistrar);
        assert(!domain.empty());
        return TypeUtil::convertQStringtoStdString(capabilitiesRegistrar->remove<TIntfProvider>(
                TypeUtil::convertStdStringtoQString(domain), provider));
    }

    /*!
     * \brief Creates a new proxy builder for the given domain and interface.
     *
     * The proxy builder is used to create a proxy object for a remote provider. It is already
     * bound to a domain and communication interface as defined in Franca. After configuration is
     * finished, ProxyBuilder::build() is called to create the proxy object.
     *
     * \tparam TIntfProxy The interface class of the proxy to create. The corresponding template
     *                    parameter of a Franca interface called "MyDemoIntf" is "MyDemoIntfProxy".
     * \param domain The domain to connect this proxy to.
     * \return A proxy builder object that can be used to create proxies. The caller takes
     *         ownership of the returned object and must take care to clean up resources properly.
     */
    template <class TIntfProxy>
    ProxyBuilder<TIntfProxy>* createProxyBuilder(const std::string& domain)
    {
        if (!proxyFactory) {
            throw JoynrException("Exception in JoynrRuntime: Creating a proxy before "
                                 "startMessaging was called is not yet supported.");
        }
        ProxyBuilder<TIntfProxy>* builder =
                new ProxyBuilder<TIntfProxy>(proxyFactory,
                                             *discoveryProxy,
                                             TypeUtil::convertStdStringtoQString(domain),
                                             dispatcherAddress,
                                             messageRouter);
        return builder;
    }

    static JoynrRuntime* createRuntime(const QString& pathToLibjoynrSettings,
                                       const QString& pathToMessagingSettings = "");

protected:
    ProxyFactory* proxyFactory;
    QSharedPointer<ParticipantIdStorage> participantIdStorage;
    CapabilitiesRegistrar* capabilitiesRegistrar;
    SystemServicesSettings systemServicesSettings;
    QSharedPointer<joynr::system::Address> dispatcherAddress;
    QSharedPointer<MessageRouter> messageRouter;
    LocalDiscoveryAggregator* discoveryProxy;
    PublicationManager* publicationManager;

private:
    DISALLOW_COPY_AND_ASSIGN(JoynrRuntime);
};

} // namespace joynr
#endif // JOYNRUNTIME_H
