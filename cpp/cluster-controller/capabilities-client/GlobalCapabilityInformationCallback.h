/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
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
#ifndef GLOBALCAPABILITYINFORMATIONCALLBACK_H
#define GLOBALCAPABILITYINFORMATIONCALLBACK_H

#include "joynr/ICallback.h"
#include "cluster-controller/capabilities-client/IGlobalCapabilitiesCallback.h"
#include "joynr/types/CapabilityInformation.h"

#include <QList>
#include <QSharedPointer>

namespace joynr
{

class GlobalCapabilityInformationCallback : public ICallback<types::CapabilityInformation>
{
public:
    GlobalCapabilityInformationCallback(QSharedPointer<IGlobalCapabilitiesCallback> igc);
    virtual ~GlobalCapabilityInformationCallback();
    virtual void onFailure(const RequestStatus status);
    virtual void onSuccess(const RequestStatus status, types::CapabilityInformation result);

private:
    QSharedPointer<IGlobalCapabilitiesCallback> callback;
};

} // namespace joynr

#endif // CAPABILITIESRESULTCALLBACK_H
