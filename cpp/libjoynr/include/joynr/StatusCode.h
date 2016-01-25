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
#ifndef STATUSCODE_H
#define STATUSCODE_H

#include "joynr/JoynrExport.h"

#include <cstdint>
#include <string>

#ifdef WIN32
#ifdef ERROR
// QT5.1 leaks this global definition from windows.h
// Because Joynr does not use windows.h directly it is safe to
// undefine this macro
#undef ERROR
#endif
#endif

namespace joynr
{

/**
 * @brief This class contains all the possible status codes
 *
 */
class JOYNR_EXPORT StatusCode
{
public:
    // success
    static StatusCode SUCCESS;

    // in progress
    static StatusCode IN_PROGRESS;

    // error states
    static StatusCode ERROR;

    /**
     * @brief Returns the status code id.
     *
     * @return std::uint32_t
     */
    std::uint32_t getId() const;

    /**
     * @brief Convenience method to print the object to String.
     *
     * @return std::string
     */
    std::string toString() const;

    bool operator==(const StatusCode& statusCode) const;
    bool operator!=(const StatusCode& statusCode) const;
    bool success() const;
    bool inProgress() const;

private:
    StatusCode(std::uint32_t id, std::string description);
    std::uint32_t id;
    std::string description;
};

} // namespace joynr
#endif // STATUSCODE_H
