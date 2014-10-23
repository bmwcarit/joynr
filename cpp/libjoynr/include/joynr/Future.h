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
#include "joynr/ICallback.h"

#include <QSemaphore>
#include <QSharedPointer>
#include <cassert>

namespace joynr
{

template <class T>
/**
 * @brief This class is used by applications to monitor the status of a request.
 * Applications can periodically ask this class for its status and retrieve
 * descriptions and codes related to the request.
 * This class also contains a method to block until a response is received.
 *
 * Applications instantiate this class and pass it to asynchronous proxy methods.
 */
class Future : public ICallback<T>
{

public:
    Future<T>()
            : callback(NULL),
              status(RequestStatusCode::IN_PROGRESS),
              result(),
              resultReceived(0),
              callbackSupplied(false)
    {
        LOG_INFO(logger,
                 QString("resultReceived.available():") +
                         QString::number(resultReceived.available()));
    }

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
     * @return T The typed value from the request.
     */
    T getValue()
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

        return result;
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
     * @brief Sets the delegating callback.  The future class acts as a wrapper to the real
     * callback class so that it can report statuses back to the application and provide
     * convenience wait methods.
     * This should be set before being wrapped into a ReplyCaller class for the messaging
     * layer.
     *
     * @param callback A shared pointer to the real application callback.
     */
    void setCallback(QSharedPointer<ICallback<T>> callback)
    {
        callbackSupplied = true;
        this->callback = callback;
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
    void onSuccess(const RequestStatus status, T result)
    {
        LOG_INFO(logger, "onSuccess has been invoked");
        this->status = RequestStatus(status);
        this->result = result;
        resultReceived.release();
        if (callbackSupplied) {
            callback->onSuccess(status, result);
        }
    }

    /**
     * @brief Callback which indicates the operation has finished and has failed.
     */
    void onFailure(const RequestStatus status)
    {
        LOG_INFO(logger, "onFailure has been invoked");
        this->status = RequestStatus(status);
        resultReceived.release();
        if (callbackSupplied) {
            callback->onFailure(status);
        }
    }

private:
    QSharedPointer<ICallback<T>> callback;
    RequestStatus status;
    T result;
    QSemaphore resultReceived;
    bool callbackSupplied;

    static joynr_logging::Logger* logger;
};

template <class T>
joynr_logging::Logger* Future<T>::logger =
        joynr_logging::Logging::getInstance()->getLogger("MSG", "Future");

template <>
/**
 * @brief The void specialisation of this class.
 *
 */
class Future<void> : public ICallback<void>
{

public:
    Future<void>()
            : callback(NULL),
              status(RequestStatusCode::IN_PROGRESS),
              resultReceived(0),
              callbackSupplied(false)
    {
    }

    RequestStatus& getStatus()
    {
        return status;
    }

    void setCallback(QSharedPointer<ICallback<void>> callback)
    {
        callbackSupplied = true;
        this->callback = callback;
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

    void onSuccess(const RequestStatus status)
    {
        this->status = RequestStatus(status);
        resultReceived.release(1);
        if (callbackSupplied) {
            callback->onSuccess(status);
        }
    }

    void onFailure(const RequestStatus status)
    {
        this->status = RequestStatus(status);
        resultReceived.release(1);
        if (callbackSupplied) {
            callback->onFailure(status);
        }
    }

private:
    QSharedPointer<ICallback<void>> callback;
    RequestStatus status;
    QSemaphore resultReceived;
    bool callbackSupplied;
};

} // namespace joynr
#endif // FUTURE_H
