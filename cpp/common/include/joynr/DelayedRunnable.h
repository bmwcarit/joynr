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
#ifndef DELAYEDRUNNABLE_H
#define DELAYEDRUNNABLE_H

#include <functional>
#include "joynr/Runnable.h"
#include "joynr/SteadyTimer.h"

namespace boost
{
namespace asio
{
class io_service;
} // namespace asio
} // namespace boost

namespace joynr
{

class DelayedRunnable
{
public:
    DelayedRunnable(std::unique_ptr<Runnable> delayedRunnable,
                    boost::asio::io_service& ioService,
                    std::chrono::milliseconds delayMs,
                    std::function<void(const boost::system::error_code&)> timerExpiredCallback)
            : timer(ioService), runnable(std::move(delayedRunnable))
    {
        timer.expiresFromNow(delayMs);
        timer.asyncWait(timerExpiredCallback);
    }

    ~DelayedRunnable()
    {
        timer.cancel();

        // Don't delete the runnable if automatic deletition is disabled
        if (runnable && !runnable->isDeleteOnExit()) {
            runnable.release();
        }
    }

    Runnable* takeRunnable()
    {
        return runnable.release();
    }

private:
    SteadyTimer timer;
    std::unique_ptr<Runnable> runnable;
};

} // namespace joynr

#endif // DELAYEDRUNNABLE_H
