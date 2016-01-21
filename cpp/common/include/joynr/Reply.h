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
#ifndef REPLY_H
#define REPLY_H

#include "joynr/JoynrCommonExport.h"
#include "joynr/exceptions/JoynrException.h"

#include <string>
#include <vector>
#include <memory>

#include "joynr/Variant.h"

namespace joynr
{

class JOYNRCOMMON_EXPORT Reply
{
public:
    Reply& operator=(const Reply& other);
    bool operator==(const Reply& other) const;
    bool operator!=(const Reply& other) const;

    const static Reply NULL_RESPONSE;

    Reply(const Reply& other);
    Reply();

    std::string getRequestReplyId() const;
    void setRequestReplyId(const std::string& requestReplyId);

    std::vector<Variant> getResponse() const;
    void setResponse(std::vector<Variant> response);

    const Variant& getError() const;
    void setError(const Variant& error);

private:
    std::string requestReplyId;
    std::vector<Variant> response;
    Variant error;
};

} // namespace joynr
#endif // REPLY_H
