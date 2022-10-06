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
#ifndef PTRUTILS_H
#define PTRUTILS_H

#include <chrono>
#include <memory>

namespace joynr
{
namespace test
{
namespace util
{

/**
 * @brief Resets a shared_ptr and waits until the object is destructed
 */
template <typename T>
inline static void resetAndWaitUntilDestroyed(std::shared_ptr<T>& sharedPtr)
{
    std::weak_ptr<T> weakPtr = sharedPtr;
    sharedPtr.reset();
    while (weakPtr.lock()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

} // namespace util
} // namespace test
} // namespace joynr

#endif // PTRUTILS_H
