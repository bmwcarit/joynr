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
#ifndef FUTURE_H
#define FUTURE_H

#include <cstdint>
#include <functional>
#include <memory>
#include <tuple>
#include <utility>

#include "joynr/Logger.h"
#include "joynr/Semaphore.h"
#include "joynr/StatusCode.h"
#include "joynr/Util.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/exceptions/JoynrExceptionUtil.h"

namespace joynr
{

template <typename Derived>
class FutureBase
{
public:
    /**
     * @brief This is a blocking call which waits until the request finishes/an error
     * occurs/or times out.
     *
     * @param timeOut The maximum number of milliseconds to wait before this request times out
     * if no response is received.
     * @throws JoynrTimeOutException if the request does not finish in the
     * expected time.
     */
    void wait(std::int64_t timeOut)
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
        JOYNR_LOG_TRACE(logger(), "resultReceived.getStatus():{}", resultReceived.getStatus());
        resultReceived.wait();
        resultReceived.notify();
    }

    /**
     * @brief Returns the current status for the given request.
     * @return joynr::StatusCode
     */
    StatusCodeEnum getStatus() const
    {
        return status;
    }

    /**
     * @brief Returns whether the status is Ok or not.
     * @return Returns whether the status is Ok or not.
     */
    bool isOk() const
    {
        return status == StatusCodeEnum::SUCCESS;
    }

    /**
     * @brief Callback which indicates the operation has finished and has failed.
     * @param error The JoynrException describing the failure
     */
    void onError(std::shared_ptr<exceptions::JoynrException> error)
    {
        JOYNR_LOG_TRACE(logger(), "onError has been invoked");
        this->error = std::move(error);
        status = StatusCodeEnum::ERROR;
        resultReceived.notify();
    }

protected:
    FutureBase() : error(nullptr), status(StatusCodeEnum::IN_PROGRESS), resultReceived(0)
    {
    }

    void checkOk() const
    {
        if (!isOk()) {
            exceptions::JoynrExceptionUtil::throwJoynrException(*error);
        }
    }

    std::shared_ptr<exceptions::JoynrException> error;
    StatusCodeEnum status;
    Semaphore resultReceived;
    ADD_LOGGER(FutureBase)
};

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
class Future : public FutureBase<Future<Ts...>>
{

public:
    /**
     * @brief Constructor
     */
    Future<Ts...>() : FutureBase<Future<Ts...>>(), results()
    {
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
        this->wait();
        this->checkOk();

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
    void get(std::int64_t timeOut, Ts&... values)
    {
        this->wait(timeOut);
        this->checkOk();

        copyResults(std::tie(values...), std::index_sequence_for<Ts...>{});
    }

    /**
     * @brief Callback which indicates the operation has finished and is successful.
     * @param results The result of the operation, type T
     */
    void onSuccess(Ts... results)
    {
        JOYNR_LOG_TRACE(this->logger(), "onSuccess has been invoked");
        this->status = StatusCodeEnum::SUCCESS;
        // transform variadic templates into a std::tuple
        this->results = std::make_tuple(std::move(results)...);
        this->resultReceived.notify();
    }

private:
    std::tuple<Ts...> results;
};

template <>
/**
 * @brief Class specialization of the void Future class.
 */
class Future<void> : public FutureBase<Future<void>>
{

public:
    Future<void>() = default;

    /**
     * @brief This is a blocking call which waits until the request finishes/an error
     * occurs/or times out. If an error occurs, a JoynrException is thrown.
     *
     * @throws JoynrException if the request is not successful
     */
    void get()
    {
        this->wait();
        this->checkOk();
    }

    /**
     * @brief This is a blocking call which waits until the request finishes/an error
     * occurs/or times out. If the request finishes successfully, it retrieves the return value for
     *the request if one exists, otherwise a JoynrException is thrown.
     *
     * @param timeOut The maximum number of milliseconds to wait before this request times out
     * @throws JoynrException if the request is not successful
     */
    void get(std::int64_t timeOut)
    {
        this->wait(timeOut);
        this->checkOk();
    }

    /**
     * @brief Callback which indicates the operation has finished and is successful.
     */
    void onSuccess()
    {
        this->status = StatusCodeEnum::SUCCESS;
        this->resultReceived.notify();
    }
};

template <typename T>
class Future<std::unique_ptr<T>> : public FutureBase<std::unique_ptr<T>>
{
public:
    void get(std::unique_ptr<T>& value)
    {
        this->wait();
        this->checkOk();

        value = std::move(result);
    }

    void get(std::int64_t timeOut, std::unique_ptr<T>& value)
    {
        this->wait(timeOut);
        this->checkOk();

        value = std::move(result);
    }

    void onSuccess(std::unique_ptr<T> value)
    {
        result = std::move(value);
        this->status = StatusCodeEnum::SUCCESS;
        this->resultReceived.notify();
    }

private:
    std::unique_ptr<T> result;
};

} // namespace joynr
#endif // FUTURE_H
