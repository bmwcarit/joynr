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
        : _isValid(false), _isInternalProviderRegistration(false)
{
}

void ClusterControllerCallContext::setIsValid(bool isValid)
{
    this->_isValid = isValid;
    JOYNR_LOG_TRACE(logger(), "setValid: {}", this->_isValid);
}

bool ClusterControllerCallContext::getIsValid() const
{
    return _isValid;
}

void ClusterControllerCallContext::setIsInternalProviderRegistration(
        bool isInternalProviderRegistration)
{
    this->_isInternalProviderRegistration = isInternalProviderRegistration;
    JOYNR_LOG_TRACE(logger(),
                    "setIsInternalProviderRegistration: {}",
                    this->_isInternalProviderRegistration);
}

bool ClusterControllerCallContext::getIsInternalProviderRegistration() const
{
    return _isInternalProviderRegistration;
}

void ClusterControllerCallContext::invalidate()
{
    JOYNR_LOG_TRACE(logger(), "invalidate");
    _isValid = false;
    _isInternalProviderRegistration = false;
}
} // namespace joynr
