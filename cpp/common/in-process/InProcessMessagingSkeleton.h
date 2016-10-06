/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

#include <string>

#include "joynr/JoynrCommonExport.h"
#include "joynr/IMessagingMulticastSubscriber.h"

namespace joynr
{

class JoynrMessage;

/*
  * This is a common Interface for InProcessClusterControllerMessagingSkeleton and
    InProcessLibJoynrMessagingSkeleton.
  */

class JOYNRCOMMON_EXPORT InProcessMessagingSkeleton : public IMessagingMulticastSubscriber
{
public:
    ~InProcessMessagingSkeleton() override = default;
    void transmit(JoynrMessage& message,
                  const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure)
            override = 0;

    void registerMulticastSubscription(const std::string& multicastId) override = 0;
    void unregisterMulticastSubscription(const std::string& multicastId) override = 0;

private:
};

} // namespace joynr
#endif // INPROCESSMESSAGINGSKELETON_H
