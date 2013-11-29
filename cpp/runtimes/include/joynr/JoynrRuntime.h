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
#include "joynr/CapabilitiesAggregator.h"
#include "joynr/ICapabilities.h"
#include "joynr/ProxyFactory.h"

#include <QString>
#include <QSharedPointer>
#include <cassert>

namespace joynr {

class JOYNRCLUSTERCONTROLLERRUNTIME_EXPORT JoynrRuntime {

public:

    JoynrRuntime() :
        proxyFactory(NULL),
        joynrCapabilitiesSendStub(NULL),
        capabilitiesRegistrar(NULL),
        capabilitiesAggregator(NULL)
    {
    }

    virtual ~JoynrRuntime() {}

    template <class T>
    QString registerCapability(const QString& domain, QSharedPointer<T> provider, const QString& authenticationToken) {
        assert(capabilitiesRegistrar);
        assert(domain!="");
        return capabilitiesRegistrar->registerCapability<T>(domain, provider, authenticationToken);
    }

    virtual void unregisterCapability(QString participantId) = 0;

    template <class T>
    QString unregisterCapability(const QString& domain, QSharedPointer<T> provider, const QString& authenticationToken) {
        assert(capabilitiesRegistrar);
        assert(domain!="");
        return capabilitiesRegistrar->unregisterCapability<T>(domain, provider, authenticationToken);
    }

    template <class T>
    ProxyBuilder<T>* getProxyBuilder(const QString& domain) {
        if(!proxyFactory || !joynrCapabilitiesSendStub){
            throw JoynrException("Exception in JoynrRuntime: Creating a proxy before startMessaging was called is not yet supported.");
        }
        ProxyBuilder<T>* builder = new ProxyBuilder<T>(proxyFactory, capabilitiesAggregator, domain);
        return builder;
    }

    static JoynrRuntime* createRuntime(const QString& pathToLibjoynrSettings,
                                      const QString& pathToMessagingSettings = "");

protected:

    ProxyFactory* proxyFactory;
    ICapabilities* joynrCapabilitiesSendStub;
    CapabilitiesRegistrar* capabilitiesRegistrar;
    QSharedPointer<CapabilitiesAggregator> capabilitiesAggregator;

private:
    DISALLOW_COPY_AND_ASSIGN(JoynrRuntime);
};

} // namespace joynr
#endif // JOYNRUNTIME_H
