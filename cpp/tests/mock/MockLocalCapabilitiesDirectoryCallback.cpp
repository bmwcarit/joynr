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
#include <thread>

#include "tests/mock/MockLocalCapabilitiesDirectoryCallback.h"

using namespace joynr;

MockLocalCapabilitiesDirectoryCallback::MockLocalCapabilitiesDirectoryCallback()
        : results(), semaphore(1)
{
    semaphore.wait();
}

void MockLocalCapabilitiesDirectoryCallback::capabilitiesReceived(
        const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& capabilities)
{
    this->results = std::move(capabilities);
    semaphore.notify();
}

std::vector<joynr::types::DiscoveryEntryWithMetaInfo> MockLocalCapabilitiesDirectoryCallback::getResults(
        int timeout)
{
    const int waitInterval = 20;
    for (int i = 0; i < timeout; i += waitInterval) {
        std::this_thread::sleep_for(std::chrono::milliseconds(waitInterval));
        if (semaphore.waitFor()) {
            semaphore.notify();
            return results;
        }
    }

    return results;
}

void MockLocalCapabilitiesDirectoryCallback::onError(
        const joynr::types::DiscoveryError::Enum& error)
{
    std::ignore = error;
    // ignore onError currently
}

void MockLocalCapabilitiesDirectoryCallback::clearResults()
{
    semaphore.waitFor();
    results.clear();
}

MockLocalCapabilitiesDirectoryCallback::~MockLocalCapabilitiesDirectoryCallback()
{
    results.clear();
}
