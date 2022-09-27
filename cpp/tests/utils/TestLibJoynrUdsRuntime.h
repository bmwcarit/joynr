/*
 * #%L
 * %%
 * Copyright (C) 2022 BMW Car IT GmbH
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
#ifndef TESTLIBJOYNRWEBSOCKETRUNTIME_H
#define TESTLIBJOYNRWEBSOCKETRUNTIME_H

#include <chrono>
#include <memory>

#include "tests/utils/Gtest.h"

#include "joynr/Semaphore.h"
#include "joynr/Settings.h"
#include "joynr/exceptions/JoynrException.h"
#include "runtimes/libjoynr-runtime/uds/LibJoynrUdsRuntime.h"

#include "tests/JoynrTest.h"

namespace joynr
{

class TestLibJoynrUdsRuntime : public LibJoynrUdsRuntime
{

public:
    TestLibJoynrUdsRuntime(std::unique_ptr<Settings> settings,
                           std::function<void(const exceptions::JoynrRuntimeException&)>&&
                                   onFatalRuntimeError = failOnFatalRuntimeError)
            : LibJoynrUdsRuntime(std::move(settings), std::move(onFatalRuntimeError))
    {
    }

    bool connect(std::chrono::milliseconds timeoutMs)
    {
        auto semaphore = std::make_shared<Semaphore>();

        LibJoynrUdsRuntime::connect([semaphore]() { semaphore->notify(); },
                                    [](const exceptions::JoynrRuntimeException& error) {
                                        FAIL() << "LibJoynrUdsRuntime::connect failed: "
                                               << error.getMessage();
                                    });

        return semaphore->waitFor(timeoutMs);
    }
};

} // namespace joynr

#endif // TESTLIBJOYNRWEBSOCKETRUNTIME_H
