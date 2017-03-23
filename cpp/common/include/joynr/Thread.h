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
#ifndef THREAD_H
#define THREAD_H

#include <string>
#include <thread>

#include "joynr/PrivateCopyAssign.h"
#include "joynr/JoynrCommonExport.h"

namespace joynr
{

/**
 * @class Thread
 * @brief Class implementing a thread runtime
 *
 * The user of this abstract class must implement @ref run. This method will be
 * called by the hosted thread if @ref start is called.
 */
class JOYNRCOMMON_EXPORT Thread
{
public:
    /**
     * @brief Constructor
     * @param name Name of the thread
     */
    explicit Thread(const std::string& name);

    /**
     * @brief Destructor
     * @note Be sure to call @ref stop and wait for its completion before
     *      destroying this object
     */
    virtual ~Thread();

    /**
     * @brief Create the thread and start the execution of @ref run
     */
    bool start();

    /**
     * @brief Stops the execution as soon as possible
     * @note If @ref run includes any looping or blocking operations this
     *      method must be overridden to implement an interruption of @ref run.
     *      Anyway, this method must be called afterward to guarantee a clean
     *      shutdown.
     */
    virtual void stop();

protected:
    /**
     * @brief Abstract method called by the thread
     * @note If this method implements a loop or a blocking operation it must
     *      be checked if @ref stop got called. After @ref stop was called
     *      this method must immediately return.
     */
    virtual void run() = 0;

private:
    /*! Disallow copy and assign */
    DISALLOW_COPY_AND_ASSIGN(Thread);

private:
    /*! The thread itself */
    std::thread* thread;

    /*! Name of the thread */
    const std::string name;
};
} // namespace joynr

#endif // THREAD_H
