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
#ifndef IMESSAGINGSKELETON_H
#define IMESSAGINGSKELETON_H

#include <functional>
#include <memory>

#include <smrf/ByteVector.h>

namespace joynr
{

class ImmutableMessage;

namespace exceptions
{
    class JoynrRuntimeException;
} // namespace exceptions

/**
 * @class IMessagingSkeleton
 * @brief Messaging skeleton interface
 */
class IMessagingSkeleton
{
public:
    virtual ~IMessagingSkeleton() = default;

    virtual void transmit(
            std::shared_ptr<ImmutableMessage> message,
            const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure) = 0;

    virtual void onMessageReceived(smrf::ByteVector&& message) = 0;
};

} // namespace joynr
#endif // IMESSAGINGSKELETON_H
