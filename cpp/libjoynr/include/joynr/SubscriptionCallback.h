/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
#ifndef SUBSCRIPTIONCALLBACK_H
#define SUBSCRIPTIONCALLBACK_H
#include "joynr/PrivateCopyAssign.h"
#include <memory>

#include "joynr/ISubscriptionCallback.h"
#include "joynr/ISubscriptionListener.h"
#include "joynr/TrackableObject.h"
#include "joynr/joynrlogging.h"
#include "joynr/Util.h"

namespace joynr
{

/**
  * @class SubscriptionCallback
  * @brief
  */

template <typename T, typename... Ts>
class SubscriptionCallback : public ISubscriptionCallback
{
public:
    SubscriptionCallback(std::shared_ptr<ISubscriptionListener<T, Ts...>> listener)
            : listener(listener)
    {
    }

    ~SubscriptionCallback() override
    {
        LOG_TRACE(logger, "destructor: entering...");
        LOG_TRACE(logger, "destructor: leaving...");
    }

    void onError(const exceptions::JoynrRuntimeException& error) override
    {
        listener->onError(error);
    }

    void onSuccess(const T& value, const Ts&... values)
    {
        listener->onReceive(value, values...);
    }

    void timeOut()
    {
        // TODO
    }

    int getTypeId() const override
    {
        return Util::getTypeId<T, Ts...>();
    }

protected:
    std::shared_ptr<ISubscriptionListener<T, Ts...>> listener;

private:
    DISALLOW_COPY_AND_ASSIGN(SubscriptionCallback);
    static joynr_logging::Logger* logger;
};

template <typename T, typename... Ts>
joynr_logging::Logger* SubscriptionCallback<T, Ts...>::logger =
        joynr_logging::Logging::getInstance()->getLogger("MSG", "SubscriptionCallback");

} // namespace joynr
#endif // SUBSCRIPTIONCALLBACK_H
