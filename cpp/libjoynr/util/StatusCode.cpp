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
#include "joynr/StatusCode.h"
#include <stdexcept>

namespace joynr
{

std::string StatusCode::toString(StatusCodeEnum enumValue)
{
    std::string literal;
    switch (enumValue) {
    case StatusCodeEnum::SUCCESS:
        literal = std::string("SUCCESS");
        break;
    case StatusCodeEnum::IN_PROGRESS:
        literal = std::string("IN_PROGRESS");
        break;
    case StatusCodeEnum::ERROR:
        literal = std::string("ERROR");
        break;
    case StatusCodeEnum::WAIT_TIMED_OUT:
        literal = std::string("WAIT_TIMED_OUT");
        break;
    }
    if (literal.empty()) {
        throw std::invalid_argument("StatusCodeEnum: No literal found");
    }
    return literal;
}

} // namespace joynr
