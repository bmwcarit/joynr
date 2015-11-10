/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
#include "tests/utils/MockObjects.h"
#include "joynr/TimeUtils.h"

#include "joynr/joynrlogging.h"
#include <QString>

using namespace joynr;
using namespace joynr_logging;

namespace MockObjects
{
Logger* logger = Logging::getInstance()->getLogger("MSG", "MockObjects");
}

const std::string& IMockProviderInterface::INTERFACE_NAME()
{
    static const std::string INTERFACE_NAME("test/interface");
    return INTERFACE_NAME;
}

std::string MockProvider::getInterfaceName() const
{
    return INTERFACE_NAME();
}

MockRunnableWithAccuracy::MockRunnableWithAccuracy(
    bool deleteMe,
    const uint64_t delay)
    : joynr::Runnable(deleteMe),
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

    const uint64_t now_ms = TimeUtils::getCurrentMillisSinceEpoch();

    const uint64_t diff = (now_ms > est_ms) ? now_ms - est_ms : est_ms - now_ms;
    LOG_TRACE(MockObjects::logger, QString("Runnable run() is called"));
    LOG_TRACE(MockObjects::logger, QString(" ETA        : %1").arg(est_ms));
    LOG_TRACE(MockObjects::logger, QString(" current    : %1").arg(now_ms));
    LOG_TRACE(MockObjects::logger, QString(" difference : %1").arg(diff));

    if (diff <= timerAccuracyTolerance_ms)
    {
        runCalledInTime();
    }
}
