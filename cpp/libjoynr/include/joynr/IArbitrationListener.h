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
#ifndef IARBITRATIONLISTENER_H
#define IARBITRATIONLISTENER_H

#include "joynr/ArbitrationStatus.h"

#include <QString>
#include <QSharedPointer>

namespace joynr {

class EndpointAddressBase;

/*
 *  IArbitrationListener is an interface used by ProviderArbitrator
 *  to norify the AbstractProxy about a successful completion
 *  of the arbitration process.
 */
class IArbitrationListener {
public:

    virtual ~IArbitrationListener(){}
    virtual void setArbitrationStatus(ArbitrationStatus::ArbitrationStatusType arbitrationStatus) =0;
    virtual void setParticipantId(const QString& participantId) = 0;
    virtual void setEndpointAddress(QSharedPointer<EndpointAddressBase> endpointAddress) =0;
};


} // namespace joynr
#endif //IARBITRATIONLISTENER_H
