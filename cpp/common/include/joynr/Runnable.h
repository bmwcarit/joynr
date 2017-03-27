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

#include "joynr/JoynrCommonExport.h"

namespace joynr
{

/**
 * @class Runnable
 * @brief Abstract class providing work that should be done
 */
class JOYNRCOMMON_EXPORT Runnable
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
     * @brief Get free on exit flag
     * @return Flag indicating if object should be deleted after execution
     */
    bool isDeleteOnExit() const;

    /**
     * @brief Method that will be called from thread context
     * @note This method must terminate execution as soon as possible if @ref
     *      shutdown got called
     */
    virtual void run() = 0;

protected:
    /**
     * @brief Constructor
     * @param deleteOnExit Flag indicating if destructor should be called when
     *      @ref run has returned
     */
    explicit Runnable(bool deleteOnExit);

private:
    /*! Flag indicating if object should be deleted after execution */
    bool deleteOnExit;
};

} // namespace joynr

#endif // JOYNRRUNNABLE_H
