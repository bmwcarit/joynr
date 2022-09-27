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
#ifndef TESTS_MOCK_MOCKRUNNABLEWITHACCURACY_H
#define TESTS_MOCK_MOCKRUNNABLEWITHACCURACY_H

#include <cstdint>

#include "tests/utils/Gmock.h"

#include "joynr/Logger.h"
#include "joynr/Runnable.h"

class MockRunnableWithAccuracy : public joynr::Runnable
{
public:
    static const std::uint64_t timerAccuracyTolerance_ms = 200U;

    MockRunnableWithAccuracy(const std::uint64_t delay);

    MOCK_CONST_METHOD0(dtorCalled, void());
    ~MockRunnableWithAccuracy();

    MOCK_METHOD0(shutdown, void());

    MOCK_CONST_METHOD0(runCalled, void());
    MOCK_CONST_METHOD0(runCalledInTime, void());
    void run() override;

private:
    const std::uint64_t est_ms;
    ADD_LOGGER(MockRunnableWithAccuracy)
};

#endif // TESTS_MOCK_MOCKRUNNABLEWITHACCURACY_H
