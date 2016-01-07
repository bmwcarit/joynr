/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
#include "joynr/Semaphore.h"

#include <chrono>

namespace joynr
{

joynr::Semaphore::Semaphore(std::int8_t initialValue) : mutex(), condition(), counter(initialValue)
{
}

void joynr::Semaphore::wait()
{
    std::unique_lock<std::mutex> lock(mutex);
    condition.wait(lock, [this] { return counter > 0; });
    --counter;
}

void joynr::Semaphore::notify()
{
    std::unique_lock<std::mutex> lock(mutex);
    ++counter;
    lock.unlock();
    condition.notify_one();
}

bool joynr::Semaphore::waitFor(
        std::chrono::milliseconds timeoutMs /*= std::chrono::milliseconds(0)*/)
{
    std::unique_lock<std::mutex> lock(mutex);
    bool result = condition.wait_for(lock, timeoutMs, [this]() { return counter > 0; });
    if (result) {
        --counter;
    }
    return result;
}

std::size_t joynr::Semaphore::getStatus() const
{
    std::unique_lock<std::mutex> lock(mutex);
    return counter;
}
} // namespace joynr
