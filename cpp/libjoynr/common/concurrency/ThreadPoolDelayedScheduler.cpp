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
#include "joynr/ThreadPoolDelayedScheduler.h"

#include <cassert>
#include <functional>

#include <boost/asio/io_service.hpp>
#include "joynr/Runnable.h"
#include "joynr/ThreadPool.h"

namespace joynr
{

ThreadPoolDelayedScheduler::ThreadPoolDelayedScheduler(std::uint8_t numberOfThreads,
                                                       const std::string& name,
                                                       boost::asio::io_service& ioService,
                                                       std::chrono::milliseconds defaultDelayMs)
        : DelayedScheduler(
                  std::bind(&ThreadPoolDelayedScheduler::execute, this, std::placeholders::_1),
                  ioService,
                  defaultDelayMs),
          threadPool(std::make_shared<ThreadPool>(name, numberOfThreads))
{
    threadPool->init();
}

ThreadPoolDelayedScheduler::~ThreadPoolDelayedScheduler()
{
    assert(!threadPool->isRunning());
}

void ThreadPoolDelayedScheduler::execute(Runnable* runnable)
{
    threadPool->execute(runnable);
}

void ThreadPoolDelayedScheduler::shutdown()
{
    DelayedScheduler::shutdown();
    threadPool->shutdown();
}

} // namespace joynr
