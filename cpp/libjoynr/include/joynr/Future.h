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
#ifndef FUTURE_H
#define FUTURE_H

#include "joynr/RequestStatus.h"
#include "joynr/joynrlogging.h"

#include <QSemaphore>
#include <QSharedPointer>
#include <cassert>
#include <functional>
#include <joynr/Util.h>

namespace joynr
{

template <size_t... N>
struct IntegerList
{
    template <size_t M>
    struct PushBack
    {
        typedef IntegerList<N..., M> Type;
    };
};

template <size_t MAX>
struct IndexList
{
    typedef typename IndexList<MAX - 1>::Type::template PushBack<MAX>::Type Type;
};

template <>
struct IndexList<0>
{
    typedef IntegerList<> Type;
};

template <size_t... Indices, typename Tuple>
auto tupleSubset(const Tuple& tuple, IntegerList<Indices...>)
        -> decltype(std::make_tuple(std::get<Indices>(tuple)...))
{
    return std::make_tuple(std::get<Indices>(tuple)...);
}

template <typename Head, typename... Tail>
std::tuple<Tail...> tail(const std::tuple<Head, Tail...>& tuple)
{
    return tupleSubset(tuple, typename IndexList<sizeof...(Tail)>::Type());
}

template <class... Ts>
/**
 * @brief This class is used by applications to monitor the status of a request.
 * Applications can periodically ask this class for its status and retrieve
 * descriptions and codes related to the request.
 * This class also contains a method to block until a response is received.
 *
 * Applications instantiate this class and pass it to asynchronous proxy methods.
 */
class Future
{

public:
    Future<Ts...>() : status(RequestStatusCode::IN_PROGRESS), results(), resultReceived(0)
    {
        LOG_INFO(logger,
                 QString("resultReceived.available():") +
                         QString::number(resultReceived.available()));
    }

    template <typename T, typename Head, typename... Tail>
    struct ResultCopier;

    template <typename Head, typename... Tail>
    struct ResultCopier<std::tuple<Head, Tail...>, Head, Tail...>
    {
        static void copy(const std::tuple<Head, Tail...>& results, Head& value, Tail&... values)
        {
            value = std::get<0>(results);
            ResultCopier<std::tuple<Tail...>, Tail...>::copy(
                    tail<Head, Tail...>(results), values...);
        }
    };
    template <typename Head>
    struct ResultCopier<std::tuple<Head>, Head>
    {
        static void copy(const std::tuple<Head>& results, Head& value)
        {
            value = std::get<0>(results);
        }
    };

    /**
     * @brief Retrieves the return value for the request if one exists.
     *
     * To retrieve the value successfully, one must first call: waitForFinish()
     * and/or getStatus(), to check if the status is OK, before calling this method.
     * An assertion error will be thrown if waitForFinish() or getStatus()
     * has not been called first.
     *
     * This method returns immediately.
     *
     * @param Ts&... The typed return values from the request.
     */
    void getValues(Ts&... values)
    {
        if (!resultReceived.tryAcquire(1)) {
            LOG_FATAL(logger, "The request is not yet finished when calling getValue()");
            assert(false);
        }
        resultReceived.release(1);

        RequestStatusCode code = getStatus().getCode();
        if (code != RequestStatusCode::OK) {
            LOG_FATAL(logger,
                      "The request status code was not OK when calling getValue(), it was: " +
                              code.toString());
            assert(false);
        }

        ResultCopier<std::tuple<Ts...>, Ts...>::copy(results, values...);
    }

    /**
     * @brief Returns the current RequestStatus for the given request.
     *
     * @return RequestStatus
     */
    RequestStatus& getStatus()
    {
        return status;
    }

    /**
     * @brief This is a blocking call which waits until the request finishes/an error
     * occurs/or times out.
     *
     * @param timeOut The maximum number of milliseconds to wait before this request times out
     * if no response is received.
     * @return QSharedPointer<RequestStatus> Returns the RequestStatus for the completed request.
     */
    RequestStatus waitForFinished(int timeOut)
    {
        if (resultReceived.tryAcquire(1, timeOut)) {
            resultReceived.release(1);
        }
        return status;
    }

    /**
     * @brief This is a blocking call which waits until the request finishes/an error
     * occurs.  Waits indefinitely.
     *
     * @return QSharedPointer<RequestStatus> Returns the RequestStatus for the completed request.
     */
    RequestStatus waitForFinished()
    {
        LOG_INFO(logger,
                 QString("resultReceived.available():") +
                         QString::number(resultReceived.available()));
        resultReceived.acquire(1);
        resultReceived.release(1);
        return status;
    }

    /**
     * @brief Returns whether the status is Ok or not.
     *
     * @return Returns whether the status is Ok or not.
     */
    bool isOk()
    {
        return status.successful();
    }

    /**
     * @brief Callback which indicates the operation has finished and is successful.
     */
    void onSuccess(const RequestStatus& status, Ts... results)
    {
        LOG_INFO(logger, "onSuccess has been invoked");
        this->status = RequestStatus(status);
        // transform variadic templates into a std::tuple
        this->results = std::make_tuple(results...);
        resultReceived.release();
    }

    /**
     * @brief Callback which indicates the operation has finished and has failed.
     */
    void onFailure(const RequestStatus& status)
    {
        LOG_INFO(logger, "onFailure has been invoked");
        this->status = RequestStatus(status);
        resultReceived.release();
    }

private:
    RequestStatus status;
    std::tuple<Ts...> results;
    QSemaphore resultReceived;

    static joynr_logging::Logger* logger;
};

template <class... Ts>
joynr_logging::Logger* Future<Ts...>::logger =
        joynr_logging::Logging::getInstance()->getLogger("MSG", "Future");

template <>
/**
 * @brief The void specialisation of this class.
 *
 */
class Future<void>
{

public:
    Future<void>() : status(RequestStatusCode::IN_PROGRESS), resultReceived(0)
    {
    }

    RequestStatus& getStatus()
    {
        return status;
    }

    RequestStatus waitForFinished(int timeOut)
    {
        if (resultReceived.tryAcquire(1, timeOut)) {
            resultReceived.release(1);
        }
        return status;
    }

    RequestStatus waitForFinished()
    {
        resultReceived.acquire(1);
        resultReceived.release(1);
        return status;
    }

    bool isOk()
    {
        return status.successful();
    }

    void onSuccess(const RequestStatus& status)
    {
        this->status = RequestStatus(status);
        resultReceived.release(1);
    }

    void onFailure(const RequestStatus& status)
    {
        this->status = RequestStatus(status);
        resultReceived.release(1);
    }

private:
    RequestStatus status;
    QSemaphore resultReceived;
};

} // namespace joynr
#endif // FUTURE_H
