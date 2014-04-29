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
#include "cluster-controller/capabilities-client/GlobalCapabilityInformationCallback.h"
#include "joynr/joynrlogging.h"
#include <cassert>

namespace joynr {

GlobalCapabilityInformationCallback::GlobalCapabilityInformationCallback(QSharedPointer<IGlobalCapabilitiesCallback> igc) :
    callback (igc)
{
}


GlobalCapabilityInformationCallback::~GlobalCapabilityInformationCallback()
{

}

void GlobalCapabilityInformationCallback::onFailure(const RequestStatus status){
    Q_UNUSED(status);
    //TODO: handle failures.
}

void GlobalCapabilityInformationCallback::onSuccess(const RequestStatus status, types::CapabilityInformation capInfo){
    assert(status.successful());
	QList<types::CapabilityInformation> result;
	result.push_back(capInfo);
    callback->capabilitiesReceived(result);
}

} // namespace joynr
