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
#ifndef SINGLETHREADEDIOSERVICE_H
#define SINGLETHREADEDIOSERVICE_H

#include <memory>
#include <thread>

#include <boost/asio/io_service.hpp>
#include "joynr/Semaphore.h"
#include "joynr/Logger.h"

namespace joynr
{

class SingleThreadedIOService : public std::enable_shared_from_this<SingleThreadedIOService>
{
public:
    SingleThreadedIOService(std::shared_ptr<Semaphore> destructed = nullptr)
            : std::enable_shared_from_this<SingleThreadedIOService>(),
              ioService(),
              ioServiceWork(),
              ioServiceThread(),
              destructed(destructed)
    {
        JOYNR_LOG_TRACE(logger(), "Created.");
    }

    ~SingleThreadedIOService()
    {
        if (destructed) {
            destructed->notify();
        }
    }

    void start()
    {
        ioServiceWork = std::make_unique<boost::asio::io_service::work>(ioService);
        ioServiceThread = std::thread(&runIOService, shared_from_this());
        JOYNR_LOG_TRACE(logger(), "Started.");
    }

    void stop()
    {
        JOYNR_LOG_TRACE(logger(), "Stopping.");
        ioServiceWork.reset();
        ioService.stop();

        // do not join here since we could be joining ourselves
        // the destructor here will not get called until thread has ended due to
        // shared_ptr reference count

        if (std::this_thread::get_id() == ioServiceThread.get_id()) {
            ioServiceThread.detach();
            JOYNR_LOG_TRACE(logger(), "Same thread: detach!");
        } else if (ioServiceThread.joinable()) {
            JOYNR_LOG_TRACE(logger(), "Other thread: join!");
            ioServiceThread.join();
        }
    }

    boost::asio::io_service& getIOService()
    {
        return ioService;
    }

private:
    static void runIOService(std::shared_ptr<SingleThreadedIOService> singleThreadedIOService)
    {
        singleThreadedIOService->ioService.run();
    }

private:
    ADD_LOGGER(SingleThreadedIOService)
    boost::asio::io_service ioService;
    std::unique_ptr<boost::asio::io_service::work> ioServiceWork;
    std::thread ioServiceThread;
    std::shared_ptr<Semaphore> destructed;
};

} // namespace joynr
#endif // SINGLETHREADEDIOSERVICE_H
