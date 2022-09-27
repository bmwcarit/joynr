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
#ifndef STATUSCODE_H
#define STATUSCODE_H

#include <cstdint>
#include <string>

namespace joynr
{

/**
 * @brief This struct contains all the possible status codes of a Future.
 *
 */
enum class StatusCodeEnum : std::uint8_t {
    /**
     * @brief The Future was successful and onSuccess was called.
     */
    SUCCESS = 0,

    /**
     * @brief The future is still in progress.
     */
    IN_PROGRESS = 1,

    /**
     * @brief Either a time-out occured or onError was called.
     */
    ERROR = 2,

    /**
     * @brief Future::wait() timed out.
     */
    WAIT_TIMED_OUT = 3,
};

class StatusCode
{
public:
    static std::string toString(StatusCodeEnum enumValue);
};

} // namespace joynr
#endif // STATUSCODE_H
