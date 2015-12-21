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
#include "joynr/TrackableObject.h"
#include <string>
#include <sstream>

namespace joynr
{

using namespace joynr_logging;

Logger* TrackableObject::logger = Logging::getInstance()->getLogger("MSG", "TrackableObject");

int TrackableObject::instances = 0;

TrackableObject::TrackableObject()
{
    ++instances;
    std::stringstream ss;
    ss << static_cast<const void*>(this);
    std::string address = ss.str();
    LOG_TRACE(logger,
              FormatString("Creating Traceable Object at address %1 Now we have %2 instances.")
                      .arg(address)
                      .arg(instances)
                      .str());
}

TrackableObject::~TrackableObject()
{
    --instances;
    std::stringstream ss;
    ss << static_cast<const void*>(this);
    std::string address = ss.str();
    LOG_TRACE(logger,
              FormatString("Deleting Traceable Object at address %1 Now we have %2 instances.")
                      .arg(address)
                      .arg(instances)
                      .str());
}

int TrackableObject::getInstances()
{
    return TrackableObject::instances;
}

} // namespace joynr
