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

#include "joynr/Reply.h"

namespace joynr
{

Reply::Reply() : requestReplyId(), error()
{
}

Reply::Reply(BaseReply&& baseReply)
        : BaseReply::BaseReply(std::move(baseReply)), requestReplyId(), error()
{
}

const std::string& Reply::getRequestReplyId() const
{
    return requestReplyId;
}

void Reply::setRequestReplyId(const std::string& requestReplyIdLocal)
{
    this->requestReplyId = requestReplyIdLocal;
}

void Reply::setRequestReplyId(std::string&& requestReplyIdLocal)
{
    this->requestReplyId = std::move(requestReplyIdLocal);
}

std::shared_ptr<exceptions::JoynrException> Reply::getError() const
{
    return error;
}

void Reply::setError(std::shared_ptr<exceptions::JoynrException> errorLocal)
{
    this->error = std::move(errorLocal);
}

bool Reply::operator==(const Reply& other) const
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

    return requestReplyId == other.getRequestReplyId() && BaseReply::operator==(other);
}

bool Reply::operator!=(const Reply& other) const
{
    return !(*this == other);
}

} // namespace joynr
