/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
#ifndef ISUBSCRIPTIONCALLBACK_H
#define ISUBSCRIPTIONCALLBACK_H

namespace joynr
{
class SubscriptionPublication;
namespace exceptions
{
class JoynrRuntimeException;
} // namespace exceptions
  /**
    * @class ISubscriptionCallback
    * @brief
    */

class ISubscriptionCallback
{
public:
    virtual void onError(const exceptions::JoynrRuntimeException& error) = 0;
    virtual ~ISubscriptionCallback() = default;
    virtual void execute(SubscriptionPublication&& subscriptionPublication) = 0;
};

} // namespace joynr
#endif // ISUBSCRIPTIONCALLBACK_H
