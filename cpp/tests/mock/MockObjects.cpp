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
#include "tests/mock/MockObjects.h"
#include "tests/utils/TimeUtils.h"

using namespace joynr;

const std::uint32_t MockProvider::MAJOR_VERSION = 1337;
const std::uint32_t MockProvider::MINOR_VERSION = 42;

const std::string& IMockProviderInterface::INTERFACE_NAME()
{
    static const std::string INTERFACE_NAME("test/interface");
    return INTERFACE_NAME;
}

const std::string& MockProvider::getInterfaceName() const
{
    return INTERFACE_NAME();
}

MockRunnableWithAccuracy::MockRunnableWithAccuracy(
    const std::uint64_t delay)
    : Runnable(),
      est_ms(TimeUtils::getCurrentMillisSinceEpoch() + delay)
{
}

MockRunnableWithAccuracy::~MockRunnableWithAccuracy()
{
    dtorCalled();
}

void MockRunnableWithAccuracy::run()
{
    runCalled();

    const std::uint64_t now_ms = TimeUtils::getCurrentMillisSinceEpoch();

    const std::uint64_t diff = (now_ms > est_ms) ? now_ms - est_ms : est_ms - now_ms;
    JOYNR_LOG_TRACE(logger(), "Runnable run() is called");
    JOYNR_LOG_TRACE(logger(), " ETA        : {}",est_ms);
    JOYNR_LOG_TRACE(logger(), " current    : {}",now_ms);
    JOYNR_LOG_TRACE(logger(), " difference : {}",diff);

    if (diff <= timerAccuracyTolerance_ms)
    {
        runCalledInTime();
    }
}
