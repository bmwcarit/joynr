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
#ifndef PUBLICATIONINTERPRETER_H
#define PUBLICATIONINTERPRETER_H

#include <cassert>
#include <memory>
#include <functional>

#include "joynr/BasePublication.h"
#include "joynr/Util.h"
#include "joynr/exceptions/JoynrExceptionUtil.h"

namespace joynr
{

template <class... Ts>
class PublicationInterpreter
{
    using ResponseTuple = std::tuple<Ts...>;

public:
    template <typename Callback>
    static void execute(Callback& callback, BasePublication&& publication)
    {
        std::shared_ptr<exceptions::JoynrRuntimeException> error = publication.getError();
        if (error) {
            callback.onError(*error);
            return;
        }

        if (!publication.hasResponse()) {
            callback.onError(exceptions::JoynrRuntimeException(
                    "Publication object had no response, discarded message"));
            return;
        }

        ResponseTuple responseTuple;
        try {
            callGetResponse(
                    responseTuple, std::move(publication), std::index_sequence_for<Ts...>{});
        } catch (const std::exception& exception) {
            callback.onError(exceptions::JoynrRuntimeException(exception.what()));
            return;
        }

        callOnSucces(std::move(responseTuple), callback, std::index_sequence_for<Ts...>{});
    }

private:
    template <std::size_t... Indices, typename Callback>
    static void callOnSucces(ResponseTuple&& response,
                             Callback& callback,
                             std::index_sequence<Indices...>)
    {
        callback.onSuccess(std::move(std::get<Indices>(response))...);
    }

    template <std::size_t... Indices>
    static void callGetResponse(ResponseTuple& response,
                                BasePublication&& publication,
                                std::index_sequence<Indices...>)
    {
        publication.getResponse(std::get<Indices>(response)...);
    }
};

template <>
class PublicationInterpreter<void>
{
public:
    template <typename Callback>
    static void execute(Callback& callback, BasePublication&& publication)
    {
        std::shared_ptr<exceptions::JoynrRuntimeException> error = publication.getError();
        if (error) {
            callback.onError(*error);
            return;
        }
        callback.onSuccess();
    }
};

} // namespace joynr
#endif // PUBLICATIONINTERPRETER_H
