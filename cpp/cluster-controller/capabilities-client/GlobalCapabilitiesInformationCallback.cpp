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
#include "cluster-controller/capabilities-client/GlobalCapabilitiesInformationCallback.h"
#include "joynr/joynrlogging.h"
#include <cassert>

namespace joynr {

GlobalCapabilitiesInformationCallback::GlobalCapabilitiesInformationCallback(QSharedPointer<IGlobalCapabilitiesCallback> igc) :
    callback (igc)
{
}


GlobalCapabilitiesInformationCallback::~GlobalCapabilitiesInformationCallback()
{

}

void GlobalCapabilitiesInformationCallback::onFailure(const RequestStatus status){
    Q_UNUSED(status); //Failures for GlobalCapabilities Lookup are not yet handled.
    //TODO: handle failures.
}

void GlobalCapabilitiesInformationCallback::onSuccess(const RequestStatus status, QList<types::CapabilityInformation> result){
    assert(status.successful());
    callback->capabilitiesReceived(result);
}

} // namespace joynr
