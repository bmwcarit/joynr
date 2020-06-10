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
#ifndef MULTICASTSUBSCRIPTIONCALLBACK_H
#define MULTICASTSUBSCRIPTIONCALLBACK_H

#include <cassert>
#include <memory>

#include "joynr/BasePublication.h"
#include "joynr/Future.h"
#include "joynr/ISubscriptionListener.h"
#include "joynr/ISubscriptionManager.h"
#include "joynr/Logger.h"
#include "joynr/MulticastPublication.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/SubscriptionCallback.h"

namespace joynr
{

/**
  * @class MulticastSubscriptionCallback
  * @brief
  */
template <typename T, typename... Ts>
class MulticastSubscriptionCallback
        : public SubscriptionCallback<MulticastSubscriptionCallback<T, Ts...>, T, Ts...>
{
private:
    using Base = SubscriptionCallback<MulticastSubscriptionCallback<T, Ts...>, T, Ts...>;

public:
    explicit MulticastSubscriptionCallback(const std::string& subscriptionId,
                                           std::shared_ptr<Future<std::string>> future,
                                           std::weak_ptr<ISubscriptionManager> subscriptionManager)
            : Base(subscriptionId, std::move(future), subscriptionManager)
    {
    }

    void onError(const BasePublication& publication, const exceptions::JoynrRuntimeException& error)
    {
        if (auto subscriptionManagerSharedPtr = Base::_subscriptionManager.lock()) {
            const MulticastPublication& multicastPublication =
                    static_cast<const MulticastPublication&>(publication);
            std::forward_list<std::shared_ptr<ISubscriptionListenerBase>> listeners =
                    subscriptionManagerSharedPtr->getMulticastSubscriptionListeners(
                            multicastPublication.getMulticastId());
            for (const auto& listener : listeners) {
                assert(listener);
                if (listener) {
                    listener->onError(error);
                } else {
                    JOYNR_LOG_FATAL(logger(), "onError: listener is nullptr");
                }
            }
        }
    }

    template <typename Holder = T>
    std::enable_if_t<std::is_void<Holder>::value, void> onSuccess(
            const BasePublication& publication)
    {
        if (auto subscriptionManagerSharedPtr = Base::_subscriptionManager.lock()) {
            const MulticastPublication& multicastPublication =
                    static_cast<const MulticastPublication&>(publication);
            std::forward_list<std::shared_ptr<ISubscriptionListenerBase>> listeners =
                    subscriptionManagerSharedPtr->getMulticastSubscriptionListeners(
                            multicastPublication.getMulticastId());
            for (const auto& listener : listeners) {
                auto voidListener =
                        std::dynamic_pointer_cast<ISubscriptionListener<void>>(listener);
                assert(voidListener);
                if (voidListener) {
                    voidListener->onReceive();
                } else {
                    JOYNR_LOG_FATAL(logger(), "onSuccess: voidListener is nullptr");
                }
            }
        }
    }

    template <typename Holder = T>
    std::enable_if_t<!std::is_void<Holder>::value, void> onSuccess(
            const BasePublication& publication,
            const Holder& value,
            const Ts&... values)
    {
        if (auto subscriptionManagerSharedPtr = Base::_subscriptionManager.lock()) {
            const MulticastPublication& multicastPublication =
                    static_cast<const MulticastPublication&>(publication);
            std::forward_list<std::shared_ptr<ISubscriptionListenerBase>> listeners =
                    subscriptionManagerSharedPtr->getMulticastSubscriptionListeners(
                            multicastPublication.getMulticastId());
            for (const auto& listener : listeners) {
                auto typedListener =
                        std::dynamic_pointer_cast<ISubscriptionListener<T, Ts...>>(listener);
                assert(typedListener);
                if (typedListener) {
                    typedListener->onReceive(value, values...);
                } else {
                    JOYNR_LOG_FATAL(logger(), "onSuccess: typedListener is nullptr");
                }
            }
        }
    }

private:
    ADD_LOGGER(MulticastSubscriptionCallback)
    DISALLOW_COPY_AND_ASSIGN(MulticastSubscriptionCallback);
};

} // namespace joynr
#endif // MULTICASTSUBSCRIPTIONCALLBACK_H
