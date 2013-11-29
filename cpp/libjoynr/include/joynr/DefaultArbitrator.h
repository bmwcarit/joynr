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
#ifndef DEFAULTARBITRATOR_H
#define DEFAULTARBITRATOR_H
#include "joynr/PrivateCopyAssign.h"

#include "joynr/ProviderArbitrator.h"
#include <QString>
#include <QSharedPointer>

namespace joynr {

class DefaultArbitrator : public ProviderArbitrator {
public:
    DefaultArbitrator(const QString& domain,const QString& interfaceName, QSharedPointer<ICapabilities> capabilitiesStub,const DiscoveryQos &discoveryQos);
    virtual void attemptArbitration();

private:
    DISALLOW_COPY_AND_ASSIGN(DefaultArbitrator);
    virtual void receiveCapabilitiesLookupResults(const QList<CapabilityEntry> capabilityEntries);
};


} // namespace joynr
#endif //DEFAULTARBITRATOR_H
