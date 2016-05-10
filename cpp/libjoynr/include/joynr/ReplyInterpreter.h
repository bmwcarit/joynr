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
#ifndef REPLYINTERPRETER_H
#define REPLYINTERPRETER_H

#include <memory>
#include <utility>

#include "joynr/Reply.h"
#include "joynr/Util.h"
#include "joynr/exceptions/JoynrException.h"

namespace joynr
{

template <class... Ts>
class ReplyInterpreter
{
public:
    template <typename Caller>
    static void execute(Caller& caller, const Reply& reply)
    {
        const Variant& error = reply.getError();
        if (!error.isEmpty()) {
            caller.returnError(error.get<exceptions::JoynrException>());
            return;
        }

        const std::vector<Variant>& response = reply.getResponse();
        if (response.empty()) {
            caller.returnError(exceptions::JoynrRuntimeException("Reply object had no response."));
            return;
        }

        callReturnValue(response, caller, std::index_sequence_for<Ts...>{});
    }

private:
    template <std::size_t... Indices, typename Caller>
    static void callReturnValue(const std::vector<Variant>& response,
                                Caller& caller,
                                std::index_sequence<Indices...>)
    {
        caller.returnValue(util::valueOf<Ts>(response[Indices])...);
    }
};

template <>
class ReplyInterpreter<void>
{
public:
    template <typename Caller>
    static void execute(Caller& caller, const Reply& reply)
    {
        const Variant& error = reply.getError();
        if (!error.isEmpty()) {
            caller.returnError(error.get<exceptions::JoynrException>());
            return;
        }
        caller.returnValue();
    }
};

} // namespace joynr
#endif // REPLYINTERPRETER_H
