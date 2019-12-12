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
#ifndef STEADYTIMER_H
#define STEADYTIMER_H

#include <chrono>
#include <functional>
#include <memory>

#include "joynr/BoostIoserviceForwardDecl.h"

namespace boost
{
namespace system
{
class error_code;
} // namespace system
} // namespace boost

namespace joynr
{
struct AsioSteadyTimer;
class SteadyTimer
{
public:
    explicit SteadyTimer(boost::asio::io_service& io_service);
    ~SteadyTimer();
    void cancel();
    void expiresFromNow(std::chrono::milliseconds duration);
    void asyncWait(std::function<void(const boost::system::error_code&)>&& callback);

private:
    std::unique_ptr<AsioSteadyTimer> steady_timer;
};
} // namespace joynr
#endif // STEADYTIMER_H
