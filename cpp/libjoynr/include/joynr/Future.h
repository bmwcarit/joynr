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
#ifndef FUTURE_H
#define FUTURE_H

#include "joynr/Logger.h"

#include <tuple>
#include <cassert>
#include <functional>
#include <joynr/Util.h>
#include <utility>
#include <cstdint>
#include "joynr/TypeUtil.h"
#include "joynr/StatusCode.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/Semaphore.h"

namespace joynr
{

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
            : error(nullptr), status(StatusCodeEnum::IN_PROGRESS), results(), resultReceived(0)
    {
        JOYNR_LOG_INFO(logger, "resultReceived.getStatus(): {}", resultReceived.getStatus());
    }

    template <typename T>
    void copyResultsImpl(T& dest, const T& value) const
    {
        dest = value;
    }

    template <std::size_t... Indices>
    void copyResults(const std::tuple<Ts&...>& destination, std::index_sequence<Indices...>) const
    {
        auto l = {
                0,
                (void(copyResultsImpl(std::get<Indices>(destination), std::get<Indices>(results))),
                 0)...};
        std::ignore = l;
    }

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

        if (!isOk()) {
            Util::throwJoynrException(*error);
        }

        copyResults(std::tie(values...), std::index_sequence_for<Ts...>{});
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
    void get(std::uint16_t timeOut, Ts&... values)
    {
        wait(timeOut);

        if (!isOk()) {
            Util::throwJoynrException(*error);
        }

        copyResults(std::tie(values...), std::index_sequence_for<Ts...>{});
    }

    /**
     * @brief Returns the current status for the given request.
     *
     * @return joynr::StatusCode
     */
    StatusCodeEnum& getStatus()
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
    void wait(std::uint16_t timeOut)
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
        JOYNR_LOG_INFO(logger, "resultReceived.getStatus():{}", resultReceived.getStatus());
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
        return status == StatusCodeEnum::SUCCESS;
    }

    /**
     * @brief Callback which indicates the operation has finished and is successful.
     * @param results The result of the operation, type T
     */
    void onSuccess(Ts... results)
    {
        JOYNR_LOG_INFO(logger, "onSuccess has been invoked");
        status = StatusCodeEnum::SUCCESS;
        // transform variadic templates into a std::tuple
        this->results = std::make_tuple(std::move(results)...);
        resultReceived.notify();
    }

    /**
     * @brief Callback which indicates the operation has finished and has failed.
     * @param error The JoynrException describing the failure
     */
    void onError(const exceptions::JoynrException& error)
    {
        JOYNR_LOG_INFO(logger, "onError has been invoked");
        this->error.reset(error.clone());
        status = StatusCodeEnum::ERROR;
        resultReceived.notify();
    }

private:
    std::shared_ptr<exceptions::JoynrException> error;
    StatusCodeEnum status;
    std::tuple<Ts...> results;
    Semaphore resultReceived;

    ADD_LOGGER(Future);
};

template <class... Ts>
INIT_LOGGER(Future<Ts...>);

template <>
/**
 * @brief Class specialization of the void Future class.
 */
class Future<void>
{

public:
    Future<void>() : error(nullptr), status(StatusCodeEnum::IN_PROGRESS), resultReceived(0)
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

        if (!isOk()) {
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
    void get(std::uint16_t timeOut)
    {
        wait(timeOut);

        if (!isOk()) {
            Util::throwJoynrException(*error);
        }
    }

    /**
     * @brief Returns the current StatusCode for the given request.
     *
     * @return StatusCode
     */
    StatusCodeEnum& getStatus()
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
    void wait(std::uint16_t timeOut)
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
        return status == StatusCodeEnum::SUCCESS;
    }

    /**
     * @brief Callback which indicates the operation has finished and is successful.
     */
    void onSuccess()
    {
        status = StatusCodeEnum::SUCCESS;
        resultReceived.notify();
    }

    /**
     * @brief Callback which indicates the operation has finished and has failed.
     * @param error The JoynrException describing the failure
     */
    void onError(const exceptions::JoynrException& error)
    {
        this->error.reset(error.clone());
        status = StatusCodeEnum::ERROR;
        resultReceived.notify();
    }

private:
    std::shared_ptr<exceptions::JoynrException> error;
    StatusCodeEnum status;
    Semaphore resultReceived;
};

} // namespace joynr
#endif // FUTURE_H
