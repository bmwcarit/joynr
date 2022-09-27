/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2019 BMW Car IT GmbH
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

#include <chrono>
#include <cstdint>
#include <exception>
#include <future>
#include <memory>
#include <mutex>
#include <tuple>
#include <utility>

#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/StatusCode.h"

namespace joynr
{

namespace exceptions
{
class JoynrException;
} // namespace exceptions

class FutureBase
{
public:
    virtual ~FutureBase();

    /**
     * @brief This is a blocking call which waits until the request finishes/an error
     * occurs/or times out.
     *
     * @param timeOut The maximum number of milliseconds to wait before this request times out
     * if no response is received.
     * @throws JoynrTimeOutException if the request does not finish in the
     * expected time.
     */
    void wait(std::int64_t timeOut);

    /**
     * @brief This is a blocking call which waits until the request finishes/an error
     * occurs.
     */
    void wait();

    /**
     * @brief Returns the current status for the given request.
     * @return joynr::StatusCode
     */
    StatusCodeEnum getStatus() const;

    /**
     * @brief Returns whether the status is Ok or not.
     * @return Returns whether the status is Ok or not.
     */
    bool isOk() const;

    /**
     * @brief Callback which indicates the operation has finished and has failed.
     * @param error The JoynrException describing the failure
     */
    void onError(std::shared_ptr<exceptions::JoynrException> error);

protected:
    FutureBase();

    virtual void storeException(std::exception_ptr eptr) = 0;

    virtual std::future_status waitForFuture(std::int64_t timeOut) = 0;

    virtual void waitForFuture() = 0;

    ADD_LOGGER(FutureBase)
    mutable std::mutex _statusMutex;
    StatusCodeEnum _status;

private:
    DISALLOW_COPY_AND_ASSIGN(FutureBase);
};

// -------------------------------------------------------------------------------------------------

template <class... Ts>
class TaskSequencer;

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
class Future : public FutureBase
{
    friend class TaskSequencer<Ts...>;

public:
    /**
     * @brief Constructor
     */
    Future() = default;

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
        JOYNR_LOG_TRACE(logger(), "get has been invoked");
        std::tie(values...) = _resultFuture.get();
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
        wait(timeOut);
        get(values...);
    }

    /**
     * @brief Callback which indicates the operation has finished and is successful.
     * @param results The result of the operation, type T
     */
    void onSuccess(Ts... results)
    {
        JOYNR_LOG_TRACE(logger(), "onSuccess has been invoked");
        const std::lock_guard<std::mutex> lock{_statusMutex};
        try {
            _resultPromise.set_value(std::make_tuple(std::move(results)...));
            _status = StatusCodeEnum::SUCCESS;
        } catch (const std::future_error& e) {
            JOYNR_LOG_ERROR(logger(),
                            "While calling onError: future_error caught: {} [_status = {}]",
                            e.what(),
                            _status);
        }
    }

private:
    void storeException(std::exception_ptr eptr) override
    {
        _resultPromise.set_exception(std::move(eptr));
    }

    std::future_status waitForFuture(std::int64_t timeOut) override
    {
        return _resultFuture.wait_for(std::chrono::milliseconds(timeOut));
    }

    void waitForFuture() override
    {
        _resultFuture.wait();
    }

    using ResultTuple = std::tuple<Ts...>;

    std::promise<ResultTuple> _resultPromise;
    std::shared_future<ResultTuple> _resultFuture{_resultPromise.get_future()};
};

// -------------------------------------------------------------------------------------------------

template <>
/**
 * @brief Class specialization of the void Future class.
 */
class Future<void> : public FutureBase
{
    friend class TaskSequencer<void>;

public:
    Future() = default;

    /**
     * @brief This is a blocking call which waits until the request finishes/an error
     * occurs/or times out. If an error occurs, a JoynrException is thrown.
     *
     * @throws JoynrException if the request is not successful
     */
    void get()
    {
        JOYNR_LOG_TRACE(logger(), "get has been invoked");
        _resultFuture.get();
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
        wait(timeOut);
        get();
    }

    /**
     * @brief Callback which indicates the operation has finished and is successful.
     */
    void onSuccess()
    {
        JOYNR_LOG_TRACE(logger(), "onSuccess has been invoked");
        const std::lock_guard<std::mutex> lock{_statusMutex};
        try {
            _resultPromise.set_value();
            _status = StatusCodeEnum::SUCCESS;
        } catch (const std::future_error& e) {
            JOYNR_LOG_ERROR(logger(),
                            "While calling onError: future_error caught: {} [_status = {}]",
                            e.what(),
                            _status);
        }
    }

private:
    void storeException(std::exception_ptr eptr) override
    {
        _resultPromise.set_exception(std::move(eptr));
    }

    std::future_status waitForFuture(std::int64_t timeOut) override
    {
        return _resultFuture.wait_for(std::chrono::milliseconds(timeOut));
    }

    void waitForFuture() override
    {
        _resultFuture.wait();
    }

    std::promise<void> _resultPromise;
    std::shared_future<void> _resultFuture{_resultPromise.get_future()};
};

} // namespace joynr

#endif // FUTURE_H
