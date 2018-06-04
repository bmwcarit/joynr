/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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

#include "ConsumerProxy.h"

#include <chrono>
#include <thread>

void ConsumerProxy::start()
{
    JOYNR_LOG_INFO(logger(), "Start threads");
    t_method1 = std::thread(&ConsumerProxy::thread_method1, this);
    t_method2 = std::thread(&ConsumerProxy::thread_method2, this);
}

void ConsumerProxy::stop()
{
    JOYNR_LOG_INFO(logger(), "Stop threads");
    thread_exit = true;
    t_method1.join();
    t_method2.join();
}

// Thread function that repeatedly calls methodWithStringParameters1
// the input parameter is changed each time
// and is meant to trigger a response from the method callback.
void ConsumerProxy::thread_method1()
{
    std::uint16_t count = 0;
    while (!thread_exit) {
        std::this_thread::sleep_for(std::chrono::milliseconds(thread_delay_ms));
        std::string stringOut;
        std::string stringIn =
                "Method1: Test string (" + std::to_string(count++) + ") for domain: ";
        localProxy.methodWithStringParameters1(stringOut, stringIn);
        JOYNR_LOG_INFO(logger(), "{}", stringOut);
    }
    JOYNR_LOG_INFO(logger(), "Thread function exited: thread_method1");
}

// Thread function that repeatedly calls methodWithStringParameters2
// the input parameter is changed each time
// and is meant to trigger a response from the method callback.
void ConsumerProxy::thread_method2()
{
    std::uint16_t count = 0;
    while (!thread_exit) {
        std::this_thread::sleep_for(std::chrono::milliseconds(thread_delay_ms));
        std::string stringOut;
        std::string stringIn =
                "Method2: Test string (" + std::to_string(count++) + ") for domain: ";
        localProxy.methodWithStringParameters2(stringOut, stringIn);
        JOYNR_LOG_INFO(logger(), "{}", stringOut);
    }
    JOYNR_LOG_INFO(logger(), "Thread function exited: thread_method2");
}
