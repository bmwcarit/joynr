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

#include "tests/utils/Gmock.h"

#include "joynr/Runnable.h"
#include "joynr/Semaphore.h"

class MockRunnableBlocking : public joynr::Runnable
{
public:
    MockRunnableBlocking() : Runnable(), semaphore(0)
    {
    }

    MOCK_CONST_METHOD0(dtorCalled, void());
    ~MockRunnableBlocking()
    {
        dtorCalled();
    }

    MOCK_METHOD0(shutdownCalled, void());
    void shutdown()
    {
        semaphore.notify();
        shutdownCalled();
    }

    void manualShutdown()
    {
        semaphore.notify();
    }

    MOCK_CONST_METHOD0(runEntry, void());
    MOCK_CONST_METHOD0(runExit, void());
    void run()
    {
        runEntry();
        semaphore.wait();
        runExit();
    }

private:
    joynr::Semaphore semaphore;
    ADD_LOGGER(MockRunnableBlocking)
};

#endif // TESTS_MOCK_MOCKRUNNABLEBLOCKING_H
