/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
#ifndef ABSTRACTGLOBALMESSAGINGSKELETON_H
#define ABSTRACTGLOBALMESSAGINGSKELETON_H

#include <functional>
#include <memory>
#include <string>

#include <smrf/ByteVector.h>

#include "joynr/JoynrClusterControllerExport.h"

#include "joynr/IMessagingMulticastSubscriber.h"

#include "joynr/Logger.h"

namespace joynr
{

namespace exceptions
{
class JoynrRuntimeException;
} // namespace exceptions

class ImmutableMessage;
class IMessageRouter;

class JOYNRCLUSTERCONTROLLER_EXPORT AbstractGlobalMessagingSkeleton
        : public IMessagingMulticastSubscriber
{
public:
    AbstractGlobalMessagingSkeleton() = default;
    virtual ~AbstractGlobalMessagingSkeleton() = default;
    virtual void transmit(
            std::shared_ptr<ImmutableMessage> message,
            const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure) = 0;

    virtual void onMessageReceived(smrf::ByteVector&& rawMessage) = 0;

protected:
    void registerGlobalRoutingEntryIfRequired(const ImmutableMessage& message,
                                              std::shared_ptr<IMessageRouter> messageRouter,
                                              const std::string& gbid = "");

private:
    ADD_LOGGER(AbstractGlobalMessagingSkeleton)
};

} // namespace joynr
#endif // ABSTRACTGLOBALMESSAGINGSKELETON_H
