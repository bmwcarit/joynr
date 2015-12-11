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
#include "joynr/SingleThreadedDelayedScheduler.h"

#include "joynr/Runnable.h"

#include <cassert>

#include "joynr/joynrlogging.h"

namespace joynr
{

using namespace joynr::joynr_logging;
Logger* joynr::SingleThreadedDelayedScheduler::logger =
        Logging::getInstance()->getLogger("MSG", "SingleThreadedDelayedScheduler");

joynr::SingleThreadedDelayedScheduler::SingleThreadedDelayedScheduler(
        const std::string& threadName,
        std::chrono::milliseconds defaultDelayMs)
        : joynr::DelayedScheduler([this](Runnable* work) { this->queue.add(work); },
                                  defaultDelayMs),
          joynr::Thread(threadName),
          keepRunning(true),
          currentlyRunning(nullptr),
          queue()
{
    Thread::start();
}

joynr::SingleThreadedDelayedScheduler::~SingleThreadedDelayedScheduler()
{
    LOG_TRACE(logger, "Dtor called");
    assert(keepRunning == false);
}

void joynr::SingleThreadedDelayedScheduler::shutdown()
{
    LOG_TRACE(logger, "shutdown() called");

    keepRunning = false;

    queue.shutdown();

    DelayedScheduler::shutdown();

    if (currentlyRunning != NULL) {
        currentlyRunning->shutdown();
    }

    Thread::stop();
}

void joynr::SingleThreadedDelayedScheduler::run()
{
    LOG_TRACE(logger, "Starting loop");

    while (keepRunning) {
        LOG_TRACE(logger, "Waiting for work");

        Runnable* work = queue.take();

        if (work != NULL) {

            LOG_TRACE(logger, "Got work. Executing now.");

            currentlyRunning = work;
            work->run();
            currentlyRunning = NULL;

            LOG_TRACE(logger, "Finished work");

            if (work->isDeleteOnExit()) {
                delete work;
            }
        }
    }

    LOG_TRACE(logger, FormatString("End of loop. Terminating").str());
}
} // namespace joynr
