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
#ifndef TESTS_MOCK_MOCKRUNNABLEBLOCKING_H
#define TESTS_MOCK_MOCKRUNNABLEBLOCKING_H

#include <condition_variable>
#include <mutex>

#include "tests/utils/Gmock.h"

#include "joynr/Runnable.h"

class MockRunnableBlocking : public joynr::Runnable
{
public:
    MockRunnableBlocking()
        : Runnable(),
          mutex(),
          wait()
    {
    }

    MOCK_CONST_METHOD0(dtorCalled, void ());
    ~MockRunnableBlocking() { dtorCalled(); }

    MOCK_METHOD0(shutdownCalled, void ());
    void shutdown()
    {
        wait.notify_all();
        shutdownCalled();
    }

    void manualShutdown()
    {
        wait.notify_all();
    }

    MOCK_CONST_METHOD0(runEntry, void ());
    MOCK_CONST_METHOD0(runExit, void ());
    void run()
    {
        runEntry();
        std::unique_lock<std::mutex> lock(mutex);
        wait.wait(lock);
        runExit();
    }

private:
    std::mutex mutex;
    std::condition_variable wait;
};

#endif // TESTS_MOCK_MOCKRUNNABLEBLOCKING_H
