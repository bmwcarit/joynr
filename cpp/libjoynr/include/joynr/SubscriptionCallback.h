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
#include "joynr/Logger.h"
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
    explicit SubscriptionCallback(std::shared_ptr<ISubscriptionListener<T, Ts...>> listener)
            : listener(listener)
    {
    }

    ~SubscriptionCallback() override
    {
        JOYNR_LOG_TRACE(logger, "destructor: entering...");
        JOYNR_LOG_TRACE(logger, "destructor: leaving...");
    }

    void onError(const exceptions::JoynrRuntimeException& error) override
    {
        listener->onError(error);
    }

    template <typename Holder = T>
    std::enable_if_t<std::is_void<Holder>::value, void> onSuccess()
    {
        listener->onReceive();
    }

    template <typename Holder = T>
    std::enable_if_t<!std::is_void<Holder>::value, void> onSuccess(const Holder& value,
                                                                   const Ts&... values)
    {
        listener->onReceive(value, values...);
    }

    void timeOut()
    {
        // TODO
    }

    int getTypeId() const override
    {
        return util::getTypeId<T, Ts...>();
    }

protected:
    std::shared_ptr<ISubscriptionListener<T, Ts...>> listener;

private:
    DISALLOW_COPY_AND_ASSIGN(SubscriptionCallback);
    ADD_LOGGER(SubscriptionCallback);
};

template <typename T, typename... Ts>
INIT_LOGGER(SINGLE_MACRO_ARG(SubscriptionCallback<T, Ts...>));

} // namespace joynr
#endif // SUBSCRIPTIONCALLBACK_H
