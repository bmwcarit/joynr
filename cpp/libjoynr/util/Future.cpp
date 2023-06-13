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
#include "joynr/Future.h"

#include <string>

namespace joynr
{

// These functions are defined in JoynrExceptionUtil.
namespace exceptions
{
std::exception_ptr convertToExceptionPtr(const JoynrException& error);
std::exception_ptr createJoynrTimeOutException(const std::string& message);
} // namespace exceptions

FutureBase::FutureBase() : _status(StatusCodeEnum::IN_PROGRESS)
{
}

FutureBase::~FutureBase()
{
    JOYNR_LOG_TRACE(logger(), "destructor has been invoked");
    // Ensure onSuccess/onError can finish before deleting this object.
    const std::lock_guard<std::mutex> lock{_statusMutex};
}

void FutureBase::wait(int64_t timeOut)
{
    JOYNR_LOG_TRACE(logger(), "wait for future {}ms...", timeOut);
    if (waitForFuture(timeOut) != std::future_status::ready) {
        const std::lock_guard<std::mutex> lock{_statusMutex};
        // Now we have the lock, recheck the status. Avoid owning the lock too long.
        if (waitForFuture(0) != std::future_status::ready) {
            _status = StatusCodeEnum::WAIT_TIMED_OUT;
            std::rethrow_exception(
                    exceptions::createJoynrTimeOutException("Request did not finish in time"));
        }
    }
}

void FutureBase::wait()
{
    JOYNR_LOG_TRACE(logger(), "wait for future...");
    waitForFuture();
}

StatusCodeEnum FutureBase::getStatus() const
{
    JOYNR_LOG_TRACE(logger(), "getStatus has been invoked");
    const std::lock_guard<std::mutex> lock{_statusMutex};
    return _status;
}

bool FutureBase::isOk() const
{
    return getStatus() == StatusCodeEnum::SUCCESS;
}

void FutureBase::onError(std::shared_ptr<exceptions::JoynrException> error)
{
    JOYNR_LOG_TRACE(logger(), "onError has been invoked");
    const std::lock_guard<std::mutex> lock{_statusMutex};
    try {
        storeException(convertToExceptionPtr(*error));
        _status = StatusCodeEnum::ERROR;
    } catch (const std::future_error& e) {
        JOYNR_LOG_ERROR(logger(),
                        "While calling onError: future_error caught: {}"
                        " [_status = {}]",
                        e.what(),
                        static_cast<std::underlying_type_t<decltype(_status)>>(_status));
    }
}

} // namespace joynr
