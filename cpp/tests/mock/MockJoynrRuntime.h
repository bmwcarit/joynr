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
#ifndef TESTS_MOCK_MOCKJOYNRRUNTIME_H
#define TESTS_MOCK_MOCKJOYNRRUNTIME_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "joynr/JoynrRuntimeImpl.h"
#include "tests/JoynrTest.h"

class MockJoynrRuntime : public joynr::JoynrRuntimeImpl
{
public:
    MockJoynrRuntime(joynr::Settings& settings,
                     std::function<void(const joynr::exceptions::JoynrRuntimeException&)>&&
                             onFatalRuntimeError = failOnFatalRuntimeError)
            : joynr::JoynrRuntimeImpl(settings, std::move(onFatalRuntimeError))
    {
    }
    MockJoynrRuntime(std::unique_ptr<joynr::Settings> settings,
                     std::function<void(const joynr::exceptions::JoynrRuntimeException&)>&&
                             onFatalRuntimeError = failOnFatalRuntimeError)
            : joynr::JoynrRuntimeImpl(*settings, std::move(onFatalRuntimeError))
    {
    }
    MOCK_METHOD0(getMessageRouter, std::shared_ptr<joynr::IMessageRouter>());
    MOCK_METHOD0(shutdown, void());
};

#endif // TESTS_MOCK_MOCKJOYNRRUNTIME_H
