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

#include "joynr/SteadyTimer.h"

#include <cassert>
#include <utility>

#include <boost/asio/steady_timer.hpp>

namespace joynr
{

struct AsioSteadyTimer : boost::asio::steady_timer
{
    using boost::asio::steady_timer::steady_timer;
};

SteadyTimer::SteadyTimer(boost::asio::io_service& io_service)
        : _steady_timer(std::make_unique<AsioSteadyTimer>(io_service))
{
}

SteadyTimer::~SteadyTimer() = default;

void SteadyTimer::cancel()
{
    _steady_timer->cancel();
}

void SteadyTimer::expiresFromNow(std::chrono::milliseconds duration)
{
    _steady_timer->expires_from_now(duration);
}

void SteadyTimer::asyncWait(std::function<void(const boost::system::error_code&)>&& callback)
{
    assert(callback);
    _steady_timer->async_wait(std::move(callback));
}

} // namespace joynr
