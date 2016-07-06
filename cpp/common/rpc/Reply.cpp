/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

bool isReplyRegistered = Variant::registerType<Reply>("joynr.Reply");

Reply::Reply() : requestReplyId(), error(), responseVariant()
{
}

Reply::Reply(BaseReply&& baseReply)
        : BaseReply::BaseReply(std::move(baseReply)), requestReplyId(), error(), responseVariant()
{
}

const std::string& Reply::getRequestReplyId() const
{
    return requestReplyId;
}

void Reply::setRequestReplyId(const std::string& requestReplyId)
{
    this->requestReplyId = requestReplyId;
}

std::shared_ptr<exceptions::JoynrException> Reply::getError() const
{
    return error;
}

void Reply::setError(std::shared_ptr<exceptions::JoynrException> error)
{
    this->error = std::move(error);
}

bool Reply::operator==(const Reply& other) const
{
    return requestReplyId == other.getRequestReplyId() && error == other.getError() &&
           BaseReply::operator==(other);
}

bool Reply::operator!=(const Reply& other) const
{
    return !(*this == other);
}

// ====== START /// TO BE DELETED
const std::vector<Variant>& Reply::getResponseVariant() const
{
    return responseVariant;
}

void Reply::setResponseVariant(std::vector<Variant> response)
{
    this->responseVariant = std::move(response);
}
// ====== END /// TO BE DELETED
} // namespace joynr
