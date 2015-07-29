/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
#ifndef REQUESTSTATUSCODE_H
#define REQUESTSTATUSCODE_H

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
 * @brief This class contains all the possible status codes that a
 * RequestStatus could be set to for a request.
 *
 */
class JOYNR_EXPORT RequestStatusCode
{
public:
    // signals that the request has been successfully processed
    static RequestStatusCode OK;

    // signals that this request has not even started
    static RequestStatusCode NOT_STARTED;

    // the request has been initiated and is currently in progress
    static RequestStatusCode IN_PROGRESS;

    // error states
    static RequestStatusCode ERROR;
    static RequestStatusCode ERROR_TIMEOUT_DISCOVERY;
    static RequestStatusCode ERROR_TIMEOUT_WAITING_FOR_RESPONSE;
    static RequestStatusCode ERROR_CANNOT_PARSE_RETURN_VALUE;

    /**
     * @brief Returns the status code id.
     *
     * @return long
     */
    uint32_t getId() const;

    /**
     * @brief Convenience method to print the object to String.
     *
     * @return QString
     */
    std::string toString() const;

    bool operator==(const RequestStatusCode& requestStatusCode) const;
    bool operator!=(const RequestStatusCode& requestStatusCode) const;

private:
    RequestStatusCode(long id, std::string description);
    uint32_t id;
    std::string description;
};

} // namespace joynr
#endif // REQUESTSTATUSCODE_H
