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
#include <tuple>

#include "joynr/IReplyInterpreter.h"
#include "joynr/ReplyCaller.h"
#include "joynr/Reply.h"
#include "joynr/Logger.h"
#include "joynr/Util.h"

namespace joynr
{

template <class... Ts>
class ReplyInterpreter : public IReplyInterpreter
{
public:
    ReplyInterpreter() = default;

    void execute(std::shared_ptr<IReplyCaller> caller, const Reply& reply) override
    {
        assert(caller);

        std::shared_ptr<ReplyCaller<Ts...>> typedCaller =
                std::dynamic_pointer_cast<ReplyCaller<Ts...>>(caller);

        const Variant& error = reply.getError();
        if (!error.isEmpty()) {
            caller->returnError(error.get<exceptions::JoynrException>());
            return;
        }

        const std::vector<Variant>& response = reply.getResponse();
        if (response.empty()) {
            JOYNR_LOG_ERROR(logger, "Unexpected empty reply object. Calling error callback");
            caller->returnError(exceptions::JoynrRuntimeException("Reply object had no response."));
            return;
        }

        callReturnValue(response, typedCaller, std::index_sequence_for<Ts...>{});
    }

private:
    template <std::size_t... Indices>
    void callReturnValue(const std::vector<Variant>& response,
                         const std::shared_ptr<ReplyCaller<Ts...>>& typedCaller,
                         std::index_sequence<Indices...>)
    {
        typedCaller->returnValue(Util::valueOf<Ts>(response[Indices])...);
    }

    ADD_LOGGER(ReplyInterpreter);
};

template <class... Ts>
INIT_LOGGER(ReplyInterpreter<Ts...>);

template <>
class ReplyInterpreter<void> : public IReplyInterpreter
{
public:
    ReplyInterpreter() = default;

    void execute(std::shared_ptr<IReplyCaller> caller, const Reply& reply) override
    {
        assert(caller);

        const Variant& error = reply.getError();
        if (!error.isEmpty()) {
            caller->returnError(error.get<exceptions::JoynrException>());
            return;
        }

        std::shared_ptr<ReplyCaller<void>> typedCallerQsp =
                std::dynamic_pointer_cast<ReplyCaller<void>>(caller);
        typedCallerQsp->returnValue();
    }
};

} // namespace joynr
#endif // REPLYINTERPRETER_H
