/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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

#include "joynr/CallContext.h"

namespace joynr
{

void CallContext::setPrincipal(const std::string& principal)
{
    this->principal = principal;
    JOYNR_LOG_TRACE(logger(), "setPrincipal '{}'", this->principal);
}

void CallContext::setPrincipal(std::string&& principal)
{
    this->principal = std::move(principal);
    JOYNR_LOG_TRACE(logger(), "setPrincipal '{}'", this->principal);
}

const std::string& CallContext::getPrincipal() const
{
    return principal;
}

void CallContext::invalidate()
{
    JOYNR_LOG_TRACE(logger(), "invalidate");
    principal = std::string();
}

bool CallContext::operator==(const CallContext& other) const
{
    return principal == other.getPrincipal();
}

bool CallContext::operator!=(const CallContext& other) const
{
    return !(*this == other);
}

} // namespace joynr
