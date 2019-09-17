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
#ifndef UNICASTSUBSCRIPTIONCALLBACK_H
#define UNICASTSUBSCRIPTIONCALLBACK_H

#include <memory>

#include "ISubscriptionManager.h"
#include "joynr/BasePublication.h"
#include "joynr/Future.h"
#include "joynr/ISubscriptionListener.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/SubscriptionCallback.h"

namespace joynr
{

/**
  * @class UnicastSubscriptionCallback
  * @brief
  */
template <typename T, typename... Ts>
class UnicastSubscriptionCallback
        : public SubscriptionCallback<UnicastSubscriptionCallback<T, Ts...>, T, Ts...>
{
private:
    using Base = SubscriptionCallback<UnicastSubscriptionCallback<T, Ts...>, T, Ts...>;

public:
    explicit UnicastSubscriptionCallback(const std::string& subscriptionId,
                                         std::shared_ptr<Future<std::string>> future,
                                         std::weak_ptr<ISubscriptionManager> subscriptionManager)
            : Base(subscriptionId, std::move(future), subscriptionManager)
    {
    }

    void onError(const BasePublication& publication, const exceptions::JoynrRuntimeException& error)
    {
        std::ignore = publication;
        if (auto subscriptionManagerSharedPtr = Base::_subscriptionManager.lock()) {
            std::shared_ptr<ISubscriptionListenerBase> listener =
                    subscriptionManagerSharedPtr->getSubscriptionListener(Base::_subscriptionId);

            if (listener) {
                listener->onError(error);
            }
        }
    }

    template <typename Holder = T>
    std::enable_if_t<std::is_void<Holder>::value, void> onSuccess(
            const BasePublication& publication)
    {
        std::ignore = publication;
        if (auto subscriptionManagerSharedPtr = Base::_subscriptionManager.lock()) {
            auto listener = std::dynamic_pointer_cast<ISubscriptionListener<void>>(
                    subscriptionManagerSharedPtr->getSubscriptionListener(Base::_subscriptionId));

            if (listener) {
                listener->onReceive();
            }
        }
    }

    template <typename Holder = T>
    std::enable_if_t<!std::is_void<Holder>::value, void> onSuccess(
            const BasePublication& publication,
            const Holder& value,
            const Ts&... values)
    {
        std::ignore = publication;
        if (auto subscriptionManagerSharedPtr = Base::_subscriptionManager.lock()) {
            auto listener = std::dynamic_pointer_cast<ISubscriptionListener<T, Ts...>>(
                    subscriptionManagerSharedPtr->getSubscriptionListener(Base::_subscriptionId));

            if (listener) {
                listener->onReceive(value, values...);
            }
        }
    }

private:
    DISALLOW_COPY_AND_ASSIGN(UnicastSubscriptionCallback);
};

} // namespace joynr
#endif // UNICASTSUBSCRIPTIONCALLBACK_H
