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
#ifndef IMESSAGING_H
#define IMESSAGING_H

#include <functional>

#include "joynr/JoynrExport.h"

namespace joynr
{

class JoynrMessage;

namespace exceptions
{
class JoynrRuntimeException;
}

/**
 * Interface for sending joynr messages in both directions between clustercontroller and libjoynr.
 */
class JOYNR_EXPORT IMessaging
{
public:
    virtual ~IMessaging() = default;
    // MessagingSkeleton on libjoynr calls Dispatcher.receive
    // MessagingSkeleton on CC calls MessageRouter.route
    virtual void transmit(
            JoynrMessage& message,
            const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure) = 0;
};

} // namespace joynr
#endif // IMESSAGING_H
