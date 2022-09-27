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
    static void execute(Caller& caller, Reply&& reply)
    {
        std::shared_ptr<exceptions::JoynrException> error = reply.getError();
        if (error) {
            caller.returnError(error);
            return;
        }

        if (!reply.hasResponse()) {
            caller.returnError(std::make_shared<exceptions::JoynrRuntimeException>(
                    "Reply object had no response."));
            return;
        }

        std::tuple<Ts...> responseTuple;
        try {
            callGetResponse(responseTuple, std::move(reply), std::index_sequence_for<Ts...>{});
        } catch (const std::exception& exception) {
            caller.returnError(
                    std::make_shared<exceptions::JoynrRuntimeException>(exception.what()));
            return;
        }
        callReturnValue(std::move(responseTuple), caller, std::index_sequence_for<Ts...>{});
    }

private:
    template <std::size_t... Indices, typename Tuple, typename Caller>
    static void callReturnValue(Tuple&& response, Caller& caller, std::index_sequence<Indices...>)
    {
        caller.returnValue(std::move(std::get<Indices>(response))...);
    }

    template <std::size_t... Indices, typename Tuple>
    static void callGetResponse(Tuple& response, Reply&& reply, std::index_sequence<Indices...>)
    {
        reply.getResponse(std::get<Indices>(response)...);
    }
};

template <>
class ReplyInterpreter<void>
{
public:
    template <typename Caller>
    static void execute(Caller& caller, Reply&& reply)
    {
        std::shared_ptr<exceptions::JoynrException> error = reply.getError();
        if (error) {
            caller.returnError(error);
            return;
        }
        caller.returnValue();
    }
};

} // namespace joynr
#endif // REPLYINTERPRETER_H
