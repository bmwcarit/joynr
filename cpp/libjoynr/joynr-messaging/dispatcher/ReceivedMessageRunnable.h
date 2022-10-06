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
#ifndef RECEIVEDMESSAGERUNNABLE_H
#define RECEIVEDMESSAGERUNNABLE_H

#include <memory>

#include "joynr/Logger.h"
#include "joynr/ObjectWithDecayTime.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Runnable.h"

namespace joynr
{

class Dispatcher;
class ImmutableMessage;

/**
 * ReceivedMessageRunnable are used handle an incoming message via a ThreadPool.
 *
 */

class ReceivedMessageRunnable : public Runnable, public ObjectWithDecayTime
{
public:
    ReceivedMessageRunnable(std::shared_ptr<ImmutableMessage> message,
                            std::weak_ptr<Dispatcher> dispatcher);
    ~ReceivedMessageRunnable() = default;

    void shutdown() override;
    void run() override;

private:
    DISALLOW_COPY_AND_ASSIGN(ReceivedMessageRunnable);
    std::shared_ptr<ImmutableMessage> _message;
    std::weak_ptr<Dispatcher> _dispatcher;
    ADD_LOGGER(ReceivedMessageRunnable)
};

} // namespace joynr
#endif // RECEIVEDMESSAGERUNNABLE_H
