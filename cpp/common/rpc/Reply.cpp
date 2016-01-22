/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

#include <utility>

#include "joynr/exceptions/JoynrException.h"

namespace joynr
{

bool isReplyRegistered = Variant::registerType<Reply>("joynr.Reply");

const Reply Reply::NULL_RESPONSE = Reply();

Reply::Reply() : requestReplyId(), response(), error(Variant::NULL_VARIANT())
{
}
Reply::Reply(const Reply& other)
        : requestReplyId(other.getRequestReplyId()), response(other.response), error(other.error)
{
}

Reply& Reply::operator=(const Reply& other)
{
    this->requestReplyId = other.getRequestReplyId();
    this->response = other.response;
    this->error = other.error;
    return *this;
}

std::string Reply::getRequestReplyId() const
{
    return requestReplyId;
}

void Reply::setRequestReplyId(const std::string& requestReplyId)
{
    this->requestReplyId = requestReplyId;
}

std::vector<Variant> Reply::getResponse() const
{
    return response;
}

void Reply::setResponse(std::vector<Variant> response)
{
    this->response = std::move(response);
}

const Variant& Reply::getError() const
{
    return this->error;
}

void Reply::setError(const Variant& error)
{
    this->error = error;
}

bool Reply::operator==(const Reply& other) const
{
    return requestReplyId == other.getRequestReplyId() && response == other.getResponse() &&
           error == other.getError();
}

bool Reply::operator!=(const Reply& other) const
{
    return !(*this == other);
}

} // namespace joynr
