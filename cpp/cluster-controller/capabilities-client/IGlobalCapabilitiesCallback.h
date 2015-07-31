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
#ifndef CAPABILITIESRESULTCALLBACK_H
#define CAPABILITIESRESULTCALLBACK_H

#include "joynr/types/CapabilityInformation.h"
#include "joynr/joynrlogging.h"
#include <vector>

namespace joynr
{

/*
 * IGlobalCapabilitiesCallback is an old callback method for capabilities.
 * Now that Code is generated the new interface for callbacks using ICallback.h is needed
 * to access the proxy.
 * For this reason, we currently have two callbacks: One GlobalCapabilitiesInformationCallback :
 * ICallback<types::QtCapabilityInformation> for the proxy
 * and one IGlobalCapabilitiesCallback for the application. The
 * ICallback<types::QtCapabilityInformation> just calls the
 * IGlobalCapabilitiesCallback.
 * Those two callbacks should be merged into one.
 */

class IGlobalCapabilitiesCallback
{
public:
    virtual ~IGlobalCapabilitiesCallback()
    {
    }
    virtual void capabilitiesReceived(std::vector<types::CapabilityInformation> results) = 0;

private:
    static joynr_logging::Logger* logger;
};

} // namespace joynr

#endif // CAPABILITIESRESULTCALLBACK_H
