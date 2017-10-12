/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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

#include "ClusterControllerCallContext.h"

namespace joynr
{
ClusterControllerCallContext::ClusterControllerCallContext()
        : isValid(false), isInternalProviderRegistration(false)
{
}

void ClusterControllerCallContext::setIsValid(bool isValid)
{
    this->isValid = isValid;
}

bool ClusterControllerCallContext::getIsValid() const
{
    return isValid;
}

void ClusterControllerCallContext::setIsInternalProviderRegistration(
        bool isInternalProviderRegistration)
{
    this->isInternalProviderRegistration = isInternalProviderRegistration;
}

bool ClusterControllerCallContext::getIsInternalProviderRegistration() const
{
    return isInternalProviderRegistration;
}

void ClusterControllerCallContext::invalidate()
{
    isValid = false;
    isInternalProviderRegistration = false;
}
} // namespace joynr
