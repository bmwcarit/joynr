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

#include <QString>
#include <QSharedPointer>
#include <cassert>

namespace joynr {


class JOYNRCLUSTERCONTROLLERRUNTIME_EXPORT JoynrRuntime {

public:

    // NOTE: The implementation of the constructor and destructor must be inside this
    // header file because there are multiple implementations (cpp files) in folder
    // cluster-controller-runtime and libjoynr-runtime.
    JoynrRuntime(QSettings &settings) :
            proxyFactory(NULL),
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

    virtual ~JoynrRuntime() {
        delete discoveryProxy;
    }

    template <class T>
    QString registerCapability(const QString& domain, QSharedPointer<T> provider, const QString& authenticationToken) {
        assert(capabilitiesRegistrar);
        assert(domain!="");
        return capabilitiesRegistrar->add<T>(domain, provider, authenticationToken);
    }

    virtual void unregisterCapability(QString participantId) = 0;

    template <class T>
    QString unregisterCapability(const QString& domain, QSharedPointer<T> provider, const QString& authenticationToken) {
        assert(capabilitiesRegistrar);
        assert(domain!="");
        return capabilitiesRegistrar->remove<T>(domain, provider, authenticationToken);
    }

    template <class T>
    ProxyBuilder<T>* getProxyBuilder(const QString& domain) {
        if(!proxyFactory){
            throw JoynrException("Exception in JoynrRuntime: Creating a proxy before startMessaging was called is not yet supported.");
        }
        ProxyBuilder<T>* builder = new ProxyBuilder<T>(
                    proxyFactory,
                    *discoveryProxy,
                    domain,
                    dispatcherAddress,
                    messageRouter
        );
        return builder;
    }

    void addBroadcastFilter(QSharedPointer<IBroadcastFilter> filter);

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
