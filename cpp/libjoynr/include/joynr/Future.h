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
#ifndef FUTURE_H
#define FUTURE_H

#include "joynr/RequestStatus.h"
#include "joynr/joynrlogging.h"

#include <cassert>
#include <functional>
#include <joynr/Util.h>
#include <stdint.h>
#include "joynr/TypeUtil.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/Semaphore.h"

namespace joynr
{

/**
 * @brief IntegerList
 */
template <size_t... N>
struct IntegerList
{
    /**
     * @brief struct PushBack
     */
    template <size_t M>
    struct PushBack
    {
        /** @brief the Pushback's Type */
        typedef IntegerList<N..., M> Type;
    };
};

/** @brief IndexList */
template <size_t MAX>
struct IndexList
{
    /** @brief The IndexList's Type */
    typedef typename IndexList<MAX - 1>::Type::template PushBack<MAX>::Type Type;
};

/** @brief IndexList */
template <>
struct IndexList<0>
{
    /** @brief The IndexList's Type */
    typedef IntegerList<> Type;
};

/**
 * @brief Create a tuple subset by indices
 * @param tuple Input tuple to select from
 * @param indices The indices
 * @return the new tuple containing the subset
 */
template <size_t... Indices, typename Tuple>
auto tupleSubset(const Tuple& tuple, IntegerList<Indices...>)
        -> decltype(std::make_tuple(std::get<Indices>(tuple)...))
{
    return std::make_tuple(std::get<Indices>(tuple)...);
}

/**
 * @brief Create a tuple containing the tail of the input tuple
 * @param tuple Input tuple to select from
 * @return The tail of input tuple
 */
template <typename Head, typename... Tail>
std::tuple<Tail...> tail(const std::tuple<Head, Tail...>& tuple)
{
    return tupleSubset(tuple, typename IndexList<sizeof...(Tail)>::Type());
}

template <class... Ts>
/**
 * @brief Class for monitoring the status of a request by applications.
 *
 * Applications can periodically ask this class for its status and retrieve
 * descriptions and codes related to the request.
 * This class also contains a method to block until a response is received.
 *
 * Applications get an instance of this class as return value asynchronous
 * proxy method calls and attribute getters/setters.
 */
class Future
{

public:
    /**
     * @brief Constructor
     */
    Future<Ts...>()
            : error(nullptr), status(RequestStatusCode::IN_PROGRESS), results(), resultReceived(0)
    {
        LOG_INFO(logger,
                 FormatString("resultReceived.getStatus():%1")
                         .arg(resultReceived.getStatus())
                         .str());
    }

    /** @brief ResultCopier helper to copy tuple entries to function arguments */
    template <typename T, typename Head, typename... Tail>
    struct ResultCopier;

    /** @brief ResultCopier helper to copy tuple entries to function arguments */
    template <typename Head, typename... Tail>
    struct ResultCopier<std::tuple<Head, Tail...>, Head, Tail...>
    {
        /**
         * @brief Copy function copies first element of tuple results to function argument value
         *        and then recursively invoke the method with the tail of the results tuple
         *        and all function arguments expect the first
         * @param results The results to copy from
         * @param value The value to copy to
         * @param values The values to copy to
         */
        static void copy(const std::tuple<Head, Tail...>& results, Head& value, Tail&... values)
        {
            value = std::get<0>(results);
            ResultCopier<std::tuple<Tail...>, Tail...>::copy(
                    tail<Head, Tail...>(results), values...);
        }
    };

    /** @brief ResultCopier variadic template termination for one single function argument */
    template <typename Head>
    struct ResultCopier<std::tuple<Head>, Head>
    {

        /**
         * @brief Copy function copies single element of tuple results to function argument value
         * @param results The results to copy from
         * @param value destination to copy to
         */
        static void copy(const std::tuple<Head>& results, Head& value)
        {
            value = std::get<0>(results);
        }
    };

    /**
     * @brief This is a blocking call which waits until the request finishes/an error
     * occurs/or times out. If the request finishes successfully, it retrieves the return value for
     *the request if one exists, otherwise a JoynrException is thrown.
     *
     * @param values The typed return values from the request.
     * @throws JoynrException if the request is not successful
     */
    void get(Ts&... values)
    {
        wait();

        RequestStatusCode code = getStatus().getCode();
        if (code != RequestStatusCode::OK) {
            Util::throwJoynrException(*error);
        }

        ResultCopier<std::tuple<Ts...>, Ts...>::copy(results, values...);
    }

    /**
     * @brief This is a blocking call which waits until the request finishes/an error
     * occurs/or times out. If the request finishes successfully, it retrieves the return value for
     *the request if one exists, otherwise a JoynrException is thrown.
     *
     * @param timeOut The maximum number of milliseconds to wait before this request times out
     * @param values The typed return values from the request.
     * @throws JoynrException if the request is not successful
     */
    void get(uint16_t timeOut, Ts&... values)
    {
        wait(timeOut);

        RequestStatusCode code = getStatus().getCode();
        if (code != RequestStatusCode::OK) {
            Util::throwJoynrException(*error);
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
     * @throws JoynrTimeOutException if the request does not finish in the
     * expected time.
     */
    void wait(uint16_t timeOut)
    {
        if (resultReceived.waitFor(std::chrono::milliseconds(timeOut))) {
            resultReceived.notify();
        } else {
            throw exceptions::JoynrTimeOutException("Request did not finish in time");
        }
    }

    /**
     * @brief This is a blocking call which waits until the request finishes/an error
     * occurs.
     */
    void wait()
    {
        LOG_INFO(logger,
                 FormatString("resultReceived.getStatus():%1")
                         .arg(resultReceived.getStatus())
                         .str());
        resultReceived.wait();
        resultReceived.notify();
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
     * @param results The result of the operation, type T
     */
    void onSuccess(Ts... results)
    {
        LOG_INFO(logger, "onSuccess has been invoked");
        status.setCode(RequestStatusCode::OK);
        // transform variadic templates into a std::tuple
        this->results = std::make_tuple(results...);
        resultReceived.notify();
    }

    /**
     * @brief Callback which indicates the operation has finished and has failed.
     * @param status The failure status
     * @param error The JoynrException describing the failure
     */
    void onError(const RequestStatusCode& status, const exceptions::JoynrException& error)
    {
        onError(RequestStatus(status), error);
    }

    /**
     * @brief Callback which indicates the operation has finished and has failed.
     * @param status The failure status
     * @param error The JoynrException describing the failure
     */
    void onError(const RequestStatus& status, const exceptions::JoynrException& error)
    {
        LOG_INFO(logger, "onError has been invoked");
        this->error.reset(error.clone());
        this->status = status;
        resultReceived.notify();
    }

private:
    std::shared_ptr<exceptions::JoynrException> error;
    RequestStatus status;
    std::tuple<Ts...> results;
    Semaphore resultReceived;

    static joynr_logging::Logger* logger;
};

template <class... Ts>
joynr_logging::Logger* Future<Ts...>::logger =
        joynr_logging::Logging::getInstance()->getLogger("MSG", "Future");

template <>
/**
 * @brief Class specialization of the void Future class.
 */
class Future<void>
{

public:
    Future<void>() : error(nullptr), status(RequestStatusCode::IN_PROGRESS), resultReceived(0)
    {
    }

    /**
     * @brief This is a blocking call which waits until the request finishes/an error
     * occurs/or times out. If an error occurs, a JoynrException is thrown.
     *
     * @throws JoynrException if the request is not successful
     */
    void get()
    {
        wait();

        RequestStatusCode code = getStatus().getCode();
        if (code != RequestStatusCode::OK) {
            Util::throwJoynrException(*error);
        }
    }

    /**
     * @brief This is a blocking call which waits until the request finishes/an error
     * occurs/or times out. If the request finishes successfully, it retrieves the return value for
     *the request if one exists, otherwise a JoynrException is thrown.
     *
     * @param timeOut The maximum number of milliseconds to wait before this request times out
     * @throws JoynrException if the request is not successful
     */
    void get(uint16_t timeOut)
    {
        wait(timeOut);

        RequestStatusCode code = getStatus().getCode();
        if (code != RequestStatusCode::OK) {
            Util::throwJoynrException(*error);
        }
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
     * @throws JoynrTimeOutException if the request does not finish in the
     * expected time.
     */
    void wait(uint16_t timeOut)
    {
        if (resultReceived.waitFor(std::chrono::milliseconds(timeOut))) {
            resultReceived.notify();
        } else {
            throw exceptions::JoynrTimeOutException("Request did not finish in time");
        }
    }

    /**
     * @brief This is a blocking call which waits until the request finishes/an error
     * occurs.
     *
     * @return RequestStatus Returns the RequestStatus for the completed request.
     */
    void wait()
    {
        resultReceived.wait();
        resultReceived.notify();
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
    void onSuccess()
    {
        status.setCode(RequestStatusCode::OK);
        resultReceived.notify();
    }

    /**
     * @brief Callback which indicates the operation has finished and has failed.
     * @param status The failure status
     * @param error The JoynrException describing the failure
     */
    void onError(const RequestStatusCode& status, const exceptions::JoynrException& error)
    {
        onError(RequestStatus(status), error);
    }

    /**
     * @brief Callback which indicates the operation has finished and has failed.
     * @param status The failure status
     * @param error The JoynrException describing the failure
     */
    void onError(const RequestStatus& status, const exceptions::JoynrException& error)
    {
        this->error.reset(error.clone());
        this->status = status;
        resultReceived.notify();
    }

private:
    std::shared_ptr<exceptions::JoynrException> error;
    RequestStatus status;
    joynr::Semaphore resultReceived;
};

} // namespace joynr
#endif // FUTURE_H
