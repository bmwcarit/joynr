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

#include "joynr/BasePublication.h"

#include <ostream>
#include <utility>

#include "joynr/exceptions/JoynrException.h"

namespace joynr
{

BasePublication::BasePublication() : error()
{
}

BasePublication::BasePublication(BaseReply&& baseReply)
        : BaseReply::BaseReply(std::move(baseReply)), error()
{
}

bool BasePublication::operator==(const BasePublication& other) const
{
    // if error ptr do not point to the same object
    if (error != other.getError()) {
        // if exactly one of error and other.getError() is a nullptr
        if (error == nullptr || other.getError() == nullptr) {
            return false;
        }
        // compare actual objects
        if (!(*error.get() == *other.getError().get())) {
            return false;
        }
    }

    return BaseReply::operator==(other);
}

bool BasePublication::operator!=(const BasePublication& other) const
{
    return !(*this == other);
}

std::shared_ptr<exceptions::JoynrRuntimeException> BasePublication::getError() const
{
    return error;
}

void BasePublication::setError(std::shared_ptr<exceptions::JoynrRuntimeException> errorLocal)
{
    this->error = std::move(errorLocal);
}

// printing SubscriptionPublication with google-test and google-mock
void PrintTo(const BasePublication& abstractPublication, ::std::ostream* os)
{
    *os << "BasePublication{";
    *os << "error:" << abstractPublication.error ? "null" : abstractPublication.error->getMessage();
    *os << ", SKIPPED printing BaseReply";
    *os << "}";
}

} // namespace joynr
