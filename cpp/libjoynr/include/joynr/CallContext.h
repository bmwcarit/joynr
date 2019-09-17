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

#ifndef CALLCONTEXT_H
#define CALLCONTEXT_H

#include <string>

#include "joynr/JoynrExport.h"
#include "joynr/Logger.h"
#include "joynr/serializer/Serializer.h"

namespace joynr
{

/**
 * @brief Contains meta information for the current method call.
 *
 * The joynr middleware provides call context information whenever a
 * joynr middleware thread calls application code, e.g. a middleware
 * thread calling a provider implementation.
 *
 * The call context contains following information:
 *  - Calling principal: Identifies the entity which caused the provider call. This value
 *    equals the creator user id from the joynr message header field.
 */
class JOYNR_EXPORT CallContext
{
public:
    CallContext() = default;
    CallContext(CallContext&&) = default;
    CallContext& operator=(CallContext&&) = default;
    CallContext(const CallContext&) = default;
    CallContext& operator=(const CallContext&) = default;

    /**
     * @brief setPrincipal sets the principal identifier of the
     * entity that caused the current call.
     * @param principal the new principal identifier
     */
    void setPrincipal(const std::string& principal);
    void setPrincipal(std::string&& principal);

    /**
     * @brief getPrincipal gets the principal indentifier of the
     * entity that caused the current call
     * @return the principal indentifier
     */
    const std::string& getPrincipal() const;

    /**
     * @brief invalidates the call context object, i.e. clears the stored data
     */
    void invalidate();

    bool operator==(const CallContext& other) const;
    bool operator!=(const CallContext& other) const;

private:
    template <typename Archive>
    friend void serialize(Archive& archive, CallContext& callContext);

    std::string _principal;
    ADD_LOGGER(CallContext)
};

template <typename Archive>
void serialize(Archive& archive, CallContext& callContext)
{
    archive(MUESLI_NVP(callContext._principal));
}

} // namespace joynr

MUESLI_REGISTER_TYPE(joynr::CallContext, "joynr.CallContext")

#endif // CALLCONTEXT_H
