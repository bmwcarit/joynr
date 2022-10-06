/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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
#ifndef TESTS_MOCK_MOCKDELAYEDSCHEDULER_H
#define TESTS_MOCK_MOCKDELAYEDSCHEDULER_H

#include "tests/utils/Gmock.h"
#include <boost/asio.hpp>

#include "joynr/DelayedScheduler.h"

class MockDelayedScheduler : public joynr::DelayedScheduler
{
public:
    MockDelayedScheduler(boost::asio::io_service& ioService)
            : DelayedScheduler([](std::shared_ptr<joynr::Runnable>) { assert(false); },
                               ioService,
                               std::chrono::milliseconds::zero())
    {
    }

    MOCK_METHOD1(unschedule, void(joynr::DelayedScheduler::RunnableHandle));
    MOCK_METHOD2(schedule,
                 DelayedScheduler::RunnableHandle(std::shared_ptr<joynr::Runnable>,
                                                  std::chrono::milliseconds delay));
};

#endif // TESTS_MOCK_MOCKDELAYEDSCHEDULER_H
