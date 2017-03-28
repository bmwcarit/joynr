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
#ifndef INPROCESSLIBJOYNRMESSAGINGSKELETON_H
#define INPROCESSLIBJOYNRMESSAGINGSKELETON_H

#include "joynr/PrivateCopyAssign.h"

#include "joynr/JoynrExport.h"
#include "common/in-process/InProcessMessagingSkeleton.h"
#include "joynr/IDispatcher.h"

namespace joynr
{

class MessagingQos;
class JoynrMessage;

class JOYNR_EXPORT InProcessLibJoynrMessagingSkeleton : public InProcessMessagingSkeleton
{
public:
    explicit InProcessLibJoynrMessagingSkeleton(IDispatcher* dispatcher);
    ~InProcessLibJoynrMessagingSkeleton() override = default;
    void transmit(JoynrMessage& message,
                  const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure)
            override;

private:
    DISALLOW_COPY_AND_ASSIGN(InProcessLibJoynrMessagingSkeleton);
    IDispatcher* dispatcher;
};

} // namespace joynr
#endif // INPROCESSLIBJOYNRMESSAGINGSKELETON_H
