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
#ifndef REQUESTSTATUS_H
#define REQUESTSTATUS_H

#include "joynr/JoynrExport.h"

#include <string>
#include <list>

#include "joynr/RequestStatusCode.h"

namespace joynr
{

/**
 * @brief This class will house all the status information needed by callers to
 * work out what (if it did) went wrong.  It will contain a status code, which
 * are predefined Joynr middleware constants, and a description string with additional
 * information.
 */
class JOYNR_EXPORT RequestStatus
{
public:
    /**
     * @brief Creates a request status with an empty description and a default request
     * status signifying that it has not started.
     */
    RequestStatus();

    /**
     * @brief Creates a RequestStatus with the supplied status code.
     *
     * @param requestCode What the RequestStatus will be initialised to.
     */
    RequestStatus(RequestStatusCode requestCode);

    /**
     * @brief Creates a RequestStatus with the supplied status code and description.
     *
     * @param requestCode What the RequestStatus will be initialised to.
     * @param description the initial description of the RequestStatus.
     */
    RequestStatus(RequestStatusCode requestCode, const std::string& description);

    /**
     * @brief A convenience method that checks whether the request was successful.
     *
     * @return bool Was the request successful?
     */
    bool successful() const;

    /**
     * @brief Returns the status code of the request.
     *
     * @return RequestStatusCode
     */
    RequestStatusCode getCode() const;

    /**
     * @brief To update the request status code with the given code.
     *
     * @param code The RequestStatusCode to update to.
     */
    void setCode(const RequestStatusCode& code);

    /**
     * @brief Returns a detailed description of the request.
     *
     * @return std::list<std::string> A list of descriptions detailing the progress of a request.
     */
    std::list<std::string> getDescription() const;

    /**
     * @brief Adds a sentence describing a state of the request to the list of descriptions.
     *
     * @param description The detail to add to the description list.
     */
    void addDescription(const std::string& description);

    /**
     * @brief A convenience method to print this object to string.
     *
     * @return std::string The String representation of this object, to be used in error
     *messages/logging etc.
     */
    std::string toString() const;

private:
    RequestStatusCode code;
    std::list<std::string> description;
};

} // namespace joynr
#endif // REQUESTSTATUS_H
