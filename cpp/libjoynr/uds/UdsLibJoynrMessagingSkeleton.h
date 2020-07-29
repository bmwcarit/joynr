/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
#ifndef UDSLIBJOYNRMESSAGINGSKELETON_H
#define UDSLIBJOYNRMESSAGINGSKELETON_H

#include <functional>
#include <memory>

#include <smrf/ByteVector.h>

#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"

namespace joynr
{

class IMessageRouter;
class ImmutableMessage;

namespace exceptions
{
class JoynrRuntimeException;
} // namespace exceptions

class UdsLibJoynrMessagingSkeleton
{
public:
    explicit UdsLibJoynrMessagingSkeleton(std::weak_ptr<IMessageRouter> messageRouter);

    ~UdsLibJoynrMessagingSkeleton() = default;

    void transmit(std::shared_ptr<ImmutableMessage> message,
                  const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure);

    void onMessageReceived(smrf::ByteVector&& message, const std::string& creator);

private:
    DISALLOW_COPY_AND_ASSIGN(UdsLibJoynrMessagingSkeleton);
    ADD_LOGGER(UdsLibJoynrMessagingSkeleton)

    std::weak_ptr<IMessageRouter> _messageRouter;
};

} // namespace joynr
#endif // UDSLIBJOYNRMESSAGINGSKELETON_H
