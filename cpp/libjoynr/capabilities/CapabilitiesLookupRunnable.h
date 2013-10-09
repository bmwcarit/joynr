/*
 * #%L
 * joynr::C++
 * $Id:$
 * $HeadURL:$
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
#ifndef CAPABILITIESLOOKUPRUNNABLE_H
#define CAPABILITIESLOOKUPRUNNABLE_H

#include "joynr/PrivateCopyAssign.h"
#include "joynr/types/ProviderQosRequirements.h"

#include <QString>
#include <QSharedPointer>
#include <QRunnable>

namespace joynr {

class ILocalCapabilitiesCallback;
class ICapabilities;
class IRequestCallerDirectory;
class DiscoveryQos;


class CapabilitiesLookupRunnable : public QRunnable {
public:
    CapabilitiesLookupRunnable(const DiscoveryQos& discoveryQos,
                               QSharedPointer<ILocalCapabilitiesCallback> callback,
                               ICapabilities *capabilitiesStub,
                               IRequestCallerDirectory* requestCallerDirectory);
    virtual ~CapabilitiesLookupRunnable(){}
    virtual void run() = 0;
protected:
    DISALLOW_COPY_AND_ASSIGN(CapabilitiesLookupRunnable);
    const DiscoveryQos& discoveryQos;
    QSharedPointer<ILocalCapabilitiesCallback> callback;
    ICapabilities *capabilitiesStub;
    IRequestCallerDirectory* requestCallerDirectory;

};

class CapabilitiesLookupByIdRunnable : public CapabilitiesLookupRunnable {

public:
    CapabilitiesLookupByIdRunnable(const QString& participantId,
                               const DiscoveryQos& discoveryQos,
                               QSharedPointer<ILocalCapabilitiesCallback> callback,
                               ICapabilities *capabilitiesStub,
                               IRequestCallerDirectory *requestCallerDirectory);
    ~CapabilitiesLookupByIdRunnable(){}
    void run();

private:
    QString participantId;
};

class CapabilitiesLookupByInterfaceRunnable : public CapabilitiesLookupRunnable {
public:
    CapabilitiesLookupByInterfaceRunnable(const QString& domain,
                                   const QString& interfaceName,
                                   const types::ProviderQosRequirements& qos,
                                   const DiscoveryQos& discoveryQos,
                                   QSharedPointer<ILocalCapabilitiesCallback> callback,
                                   ICapabilities *capabilitiesStub,
                                   IRequestCallerDirectory *requestCallerDirectory);
    ~CapabilitiesLookupByInterfaceRunnable(){}
    void run();
private:
    QString domain;
    QString interfaceName;
    types::ProviderQosRequirements qos;
};

} // namespace joynr
#endif //CAPABILITIESLOOKUPRUNNABLE_H
