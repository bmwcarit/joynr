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
#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>

#include "joynr/JoynrExport.h"
#include "joynr/PrivateCopyAssign.h"

namespace joynr
{
/**
 * @class Semaphore
 * @brief A countable concurrency lock
 */
class JOYNR_EXPORT Semaphore
{
public:
    /**
     * @brief Constructor
     * @param initialValue Initial counter value
     * @note If the initial counter value is set to @c 0 the first call to
     *      @ref wait will block
     */
    explicit Semaphore(std::size_t initialValue = 0);

    /**
     * @brief Destructor
     */
    ~Semaphore() = default;

    /**
     * @brief Locks the semaphore by decreasing the internal counter. If the
     *      internal counter is already @c 0 this method will block.
     */
    void wait();

    /**
     * @brief Increases the internal counter and notifies tasks that are
     *      waiting in @ref wait or @ref waitFor
     */
    void notify();

    /**
     * @brief Locks the semaphore by decreasing the internal counter. If the
     *      internal counter is already @c 0 this method will block for the
     *      given amount of time.
     * @param timeoutMs Number of milliseconds this method is allowed to block
     * @return If @c true the semaphore is accquired, @c false on timeout
     */
    bool waitFor(std::chrono::milliseconds timeoutMs = std::chrono::milliseconds::zero());

    /**
     * Returns the current status of the internal counter
     * @return Value of the internal counter
     */
    std::size_t getStatus() const;

private:
    DISALLOW_COPY_AND_ASSIGN(Semaphore);

    /*! Mutex */
    mutable std::mutex mutex;
    /*! Condition variable */
    std::condition_variable condition;
    /*! Counter */
    std::size_t counter;
};

} // namespace joynr

#endif // SEMAPHORE_H
