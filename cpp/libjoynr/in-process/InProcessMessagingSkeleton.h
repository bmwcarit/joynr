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
#ifndef INPROCESSMESSAGINGSKELETON_H
#define INPROCESSMESSAGINGSKELETON_H

#include <functional>

#include "joynr/IMessaging.h"
#include "joynr/JoynrExport.h"
#include "joynr/PrivateCopyAssign.h"

namespace joynr
{

class IDispatcher;
class JoynrMessage;

namespace exceptions
{
class JoynrRuntimeException;
} // namespace exceptions

class JOYNR_EXPORT InProcessMessagingSkeleton : public IMessaging
{
public:
    explicit InProcessMessagingSkeleton(IDispatcher* dispatcher);
    ~InProcessMessagingSkeleton() override = default;
    void transmit(JoynrMessage& message,
                  const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure)
            override;

private:
    DISALLOW_COPY_AND_ASSIGN(InProcessMessagingSkeleton);
    IDispatcher* dispatcher;
};

} // namespace joynr
#endif // INPROCESSMESSAGINGSKELETON_H
