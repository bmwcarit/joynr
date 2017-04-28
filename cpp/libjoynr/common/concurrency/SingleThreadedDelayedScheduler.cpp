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
#include "joynr/SingleThreadedDelayedScheduler.h"

#include <boost/asio/io_service.hpp>
#include <boost/system/error_code.hpp>
#include "joynr/Runnable.h"

namespace joynr
{

INIT_LOGGER(SingleThreadedDelayedScheduler);

SingleThreadedDelayedScheduler::SingleThreadedDelayedScheduler(
        const std::string& threadName,
        boost::asio::io_service& ioService,
        std::chrono::milliseconds defaultDelayMs)
        : DelayedScheduler([this](Runnable* work) { this->queue.add(work); },
                           ioService,
                           defaultDelayMs),
          Thread(threadName),
          keepRunning(true),
          currentlyRunning(nullptr),
          queue(),
          mutex()
{
    Thread::start();
}

SingleThreadedDelayedScheduler::~SingleThreadedDelayedScheduler()
{
    JOYNR_LOG_TRACE(logger, "Dtor called");
    shutdown();
}

void SingleThreadedDelayedScheduler::shutdown()
{
    JOYNR_LOG_TRACE(logger, "shutdown() called");

    keepRunning = false;

    DelayedScheduler::shutdown();

    queue.shutdown();

    {
        std::lock_guard<std::mutex> lock(mutex);
        if (currentlyRunning != nullptr) {
            currentlyRunning->shutdown();
        }
    }

    Thread::stop();
}

void SingleThreadedDelayedScheduler::run()
{
    JOYNR_LOG_TRACE(logger, "Starting loop");

    while (keepRunning) {
        JOYNR_LOG_TRACE(logger, "Waiting for work");

        Runnable* work = queue.take();

        if (work != nullptr) {

            JOYNR_LOG_TRACE(logger, "Got work. Executing now.");
            {
                std::lock_guard<std::mutex> lock(mutex);
                if (!keepRunning) {
                    if (work->isDeleteOnExit()) {
                        delete work;
                    }
                    break;
                }
                currentlyRunning = work;
            }
            work->run();
            {
                std::lock_guard<std::mutex> lock(mutex);
                currentlyRunning = nullptr;
            }

            JOYNR_LOG_TRACE(logger, "Finished work");

            if (work->isDeleteOnExit()) {
                delete work;
            }
        }
    }

    JOYNR_LOG_TRACE(logger, "End of loop. Terminating");
}
} // namespace joynr
