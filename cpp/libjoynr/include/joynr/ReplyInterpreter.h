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
#ifndef REPLYINTERPRETER_H
#define REPLYINTERPRETER_H

#include "joynr/IReplyInterpreter.h"
#include "joynr/ReplyCaller.h"
#include "joynr/Reply.h"
#include "joynr/joynrlogging.h"
#include "joynr/JsonSerializer.h"
#include "joynr/Util.h"

namespace joynr
{

template <class... Ts>
class ReplyInterpreter : public IReplyInterpreter
{
public:
    ReplyInterpreter()
    {
    }

    void execute(QSharedPointer<IReplyCaller> caller, const Reply& reply)
    {
        assert(!caller.isNull());

        QSharedPointer<ReplyCaller<Ts...>> typedCallerQsp =
                caller.dynamicCast<ReplyCaller<Ts...>>();

        if ((reply.getResponse()).isEmpty()) {
            LOG_ERROR(logger, QString("reply object has no response, discarding message"));
            typedCallerQsp->timeOut();
            return;
        }

        std::tuple<Ts...> values = Util::toValueTuple<Ts...>(reply.getResponse());
        auto func = std::mem_fn(&ReplyCaller<Ts...>::returnValue);

        Util::expandTupleIntoFunctionArguments(func, typedCallerQsp, values);
    }

private:
    static joynr_logging::Logger* logger;
};

template <class... Ts>
joynr_logging::Logger* ReplyInterpreter<Ts...>::logger =
        joynr_logging::Logging::getInstance()->getLogger("MSG", "ReplyInterpreter");

template <>
class ReplyInterpreter<void> : public IReplyInterpreter
{
public:
    ReplyInterpreter()
    {
        qRegisterMetaType<Reply>();
    }

    void execute(QSharedPointer<IReplyCaller> caller, const Reply& reply)
    {
        assert(!caller.isNull());
        Q_UNUSED(reply); // the reply should be empty, and is just passed in to match the common
                         // interface
        QSharedPointer<ReplyCaller<void>> typedCallerQsp = caller.dynamicCast<ReplyCaller<void>>();
        typedCallerQsp->returnValue();
    }
};

} // namespace joynr
#endif // REPLYINTERPRETER_H
