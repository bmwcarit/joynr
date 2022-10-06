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
#ifndef UDSMESSAGINGSTUB_H
#define UDSMESSAGINGSTUB_H

#include <functional>
#include <memory>

#include "joynr/IMessagingStub.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"

namespace joynr
{

class ImmutableMessage;
class IUdsSender;

namespace exceptions
{
class JoynrRuntimeException;
} // namespace exceptions

/**
 * @class UdsMessagingStub
 * @brief Represents an outgoing uds connection
 */
class UdsMessagingStub final : public IMessagingStub
{
public:
    /**
     * @brief Constructor
     * @param uds Interface to be used to send data
     */
    UdsMessagingStub(std::shared_ptr<IUdsSender> udsSender);

    /**
     * @brief Destructor
     */
    ~UdsMessagingStub() = default;
    void transmit(std::shared_ptr<ImmutableMessage> message,
                  const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure);

private:
    DISALLOW_COPY_AND_ASSIGN(UdsMessagingStub);

    /*! Message sender for outgoing messages over uds */
    std::shared_ptr<IUdsSender> _udsSender;

    ADD_LOGGER(UdsMessagingStub)
};

} // namespace joynr
#endif // UDSMESSAGINGSTUB_H
