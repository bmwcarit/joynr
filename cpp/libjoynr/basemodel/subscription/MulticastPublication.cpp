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

#include "joynr/MulticastPublication.h"
#include "joynr/exceptions/JoynrException.h"

namespace joynr
{

MulticastPublication::MulticastPublication() : BasePublication(), multicastId()
{
}

MulticastPublication::MulticastPublication(BaseReply&& baseReply)
        : BasePublication(std::move(baseReply)), multicastId()
{
}

const std::string& MulticastPublication::getMulticastId() const
{
    return multicastId;
}

void MulticastPublication::setMulticastId(const std::string& multicastIdPararm)
{
    this->multicastId = multicastIdPararm;
}

void MulticastPublication::setMulticastId(std::string&& multicastIdPararm)
{
    this->multicastId = std::move(multicastIdPararm);
}

bool MulticastPublication::operator==(const MulticastPublication& other) const
{
    return multicastId == other.getMulticastId() && BasePublication::operator==(other);
}

bool MulticastPublication::operator!=(const MulticastPublication& other) const
{
    return !(*this == other);
}

// printing MulticastPublication with google-test and google-mock
void PrintTo(const MulticastPublication& multicastPublication, ::std::ostream* os)
{
    *os << "MulticastPublication{";
    *os << "multicastId:" << multicastPublication.multicastId;
    *os << ", ";
    *os << "error:" << multicastPublication.getError()
            ? "null"
            : multicastPublication.getError()->getMessage();
    *os << ", SKIPPED printing BaseReply";
    *os << "}";
}

} // namespace joynr
