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
#ifndef JOYNRRUNNABLE_H
#define JOYNRRUNNABLE_H

#include <memory>

#include "joynr/JoynrExport.h"

namespace joynr
{

/**
 * @class Runnable
 * @brief Abstract class providing work that should be done
 */
class JOYNR_EXPORT Runnable : public std::enable_shared_from_this<Runnable>
{
public:
    /**
     * @brief Destructor
     */
    virtual ~Runnable() = default;

    /**
     * @brief Notifies about execution to be finished as soon as possible
     */
    virtual void shutdown() = 0;

    /**
     * @brief Method that will be called from thread context
     * @note This method must terminate execution as soon as possible if @ref
     *      shutdown got called
     */
    virtual void run() = 0;

protected:
    /**
     * @brief Constructor
     */
    explicit Runnable();
};

} // namespace joynr

#endif // JOYNRRUNNABLE_H
