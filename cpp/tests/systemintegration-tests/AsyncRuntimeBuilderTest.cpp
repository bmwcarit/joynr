/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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

#include <chrono>
#include <cmath>
#include <future>
#include <memory>
#include <thread>

#include "tests/utils/Gtest.h"

#include "joynr/JoynrRuntime.h"
#include "joynr/Settings.h"

#include "tests/JoynrTest.h"

using namespace ::testing;
using namespace joynr;

class AsyncRuntimeBuilderTest : public ::testing::Test
{
protected:
    static std::unique_ptr<joynr::Settings> createJoynrSettings()
    {
        auto settings = std::make_unique<::joynr::Settings>(
                "test-resources/libjoynrSystemIntegration1.settings");
        Settings::merge(
                Settings{"test-resources/MqttSystemIntegrationTest1.settings"}, *settings, false);
        return settings;
    }

    template <typename Duration>
    static std::vector<Duration> createDurationSequence(const Duration& maxDuration)
    {
        assert(0 < maxDuration.count());

        std::vector<Duration> retval;
        retval.emplace_back(Duration::zero());
        const auto quotient = 1.78;
        constexpr unsigned n = 7u;
        for (auto interval = std::max(2u,
                                      unsigned(std::floor(maxDuration.count() * (quotient - 1) /
                                                          (std::pow(quotient, n) - 1))));
             interval <= maxDuration.count();
             interval *= quotient) {
            if (std::crbegin(retval)->count() != interval) {
                retval.emplace_back(Duration{interval});
            }
        }
        if (std::crbegin(retval)->count() != maxDuration.count()) {
            retval.emplace_back(maxDuration);
        }
        return retval;
    }
};

TEST_F(AsyncRuntimeBuilderTest, createRuntime)
{
    auto onSuccess = []() {};

    auto onError = [](const exceptions::JoynrRuntimeException& exception) {
        std::ignore = exception;
        FAIL();
    };

    std::shared_ptr<JoynrRuntime> runtime{JoynrRuntime::createRuntimeAsync(
            createJoynrSettings(), failOnFatalRuntimeError, onSuccess, onError)};
}

TEST_F(AsyncRuntimeBuilderTest, createRuntimeThenDelete)
{
    using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;

    std::promise<TimePoint> startupFinishedPromise;

    auto onSuccessFulfill = [&startupFinishedPromise]() {
        startupFinishedPromise.set_value(TimePoint::clock::now());
    };
    auto onSuccess = []() {};
    auto onError = [](const exceptions::JoynrRuntimeException& exception) {
        std::ignore = exception;
        FAIL();
    };

    const auto timestampStarted = TimePoint::clock::now();
    std::shared_ptr<JoynrRuntime> runtime{JoynrRuntime::createRuntimeAsync(
            createJoynrSettings(), failOnFatalRuntimeError, onSuccessFulfill, onError)};
    const auto timestampFinished = startupFinishedPromise.get_future().get();
    runtime.reset();
    const auto startupTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            timestampFinished - timestampStarted);
    for (const auto& interval : createDurationSequence(startupTime)) {
        runtime = JoynrRuntime::createRuntimeAsync(
                createJoynrSettings(), failOnFatalRuntimeError, onSuccess, onError);
        std::this_thread::sleep_for(interval);
        runtime.reset();
    }
}

TEST_F(AsyncRuntimeBuilderTest, createRuntimeTemporary)
{
    auto onSuccess = []() {};

    auto onError = [](const exceptions::JoynrRuntimeException& exception) {
        std::ignore = exception;
        FAIL();
    };

    // Do not save it to local variable.
    JoynrRuntime::createRuntimeAsync(
            createJoynrSettings(), failOnFatalRuntimeError, onSuccess, onError);
}
