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

#include "joynr/BaseReply.h"

#include <memory>

namespace joynr
{

BaseReply::BaseReply() : _response()
{
}

bool BaseReply::operator==(const BaseReply& other) const
{
    std::ignore = other;
    return true; // TODO: response == other.getResponse()
}

bool BaseReply::operator!=(const BaseReply& other) const
{
    return !(*this == other);
}

} // namespace joynr
