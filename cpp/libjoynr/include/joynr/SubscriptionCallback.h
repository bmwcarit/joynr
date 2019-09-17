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
#ifndef SUBSCRIPTIONCALLBACK_H
#define SUBSCRIPTIONCALLBACK_H

#include <memory>
#include <tuple>

#include "ISubscriptionManager.h"
#include "joynr/BasePublication.h"
#include "joynr/Future.h"
#include "joynr/ISubscriptionCallback.h"
#include "joynr/ISubscriptionListener.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/SubscriptionReply.h"

namespace joynr
{

/**
  * @class SubscriptionCallback
  * @brief
  */
template <typename Derived, typename T, typename... Ts>
class SubscriptionCallback : public ISubscriptionCallback
{
public:
    explicit SubscriptionCallback(const std::string& subscriptionId,
                                  std::shared_ptr<Future<std::string>> future,
                                  std::weak_ptr<ISubscriptionManager> subscriptionManager)
            : _subscriptionId(subscriptionId),
              _future(std::move(future)),
              _subscriptionManager(std::move(subscriptionManager))
    {
    }

    void onError(const BasePublication& publication, const exceptions::JoynrRuntimeException& error)
    {
        static_cast<Derived*>(this)->onError(publication, error);
    }

    template <typename Holder = T>
    std::enable_if_t<std::is_void<Holder>::value, void> onSuccess(
            const BasePublication& publication)
    {
        static_cast<Derived*>(this)->onSuccess(publication);
    }

    template <typename Holder = T>
    std::enable_if_t<!std::is_void<Holder>::value, void> onSuccess(
            const BasePublication& publication,
            const Holder& value,
            const Ts&... values)
    {
        static_cast<Derived*>(this)->onSuccess(publication, value, values...);
    }

    void execute(BasePublication&& publication) override
    {
        interpret(std::move(publication));
    }

    void execute(const SubscriptionReply& subscriptionReply) override
    {
        if (auto subscriptionManagerSharedPtr = _subscriptionManager.lock()) {
            std::shared_ptr<exceptions::JoynrRuntimeException> error = subscriptionReply.getError();
            std::shared_ptr<ISubscriptionListenerBase> listener =
                    subscriptionManagerSharedPtr->getSubscriptionListener(_subscriptionId);
            if (error) {
                subscriptionManagerSharedPtr->unregisterSubscription(
                        subscriptionReply.getSubscriptionId());
                _future->onError(error);

                if (listener) {
                    listener->onError(*error);
                }
            } else {
                _future->onSuccess(subscriptionReply.getSubscriptionId());

                if (listener) {
                    listener->onSubscribed(subscriptionReply.getSubscriptionId());
                }
            }
        }
    }

protected:
    std::string _subscriptionId;
    std::shared_ptr<Future<std::string>> _future;
    std::weak_ptr<ISubscriptionManager> _subscriptionManager;

private:
    DISALLOW_COPY_AND_ASSIGN(SubscriptionCallback);
    template <typename Holder = T>
    std::enable_if_t<std::is_void<Holder>::value, void> interpret(BasePublication&& publication)
    {
        std::shared_ptr<exceptions::JoynrRuntimeException> error = publication.getError();
        if (error) {
            onError(publication, *error);
            return;
        }
        onSuccess(publication);
    }

    template <typename Holder = T>
    std::enable_if_t<!std::is_void<Holder>::value, void> interpret(BasePublication&& publication)
    {
        std::shared_ptr<exceptions::JoynrRuntimeException> error = publication.getError();
        if (error) {
            onError(publication, *error);
            return;
        }

        if (!publication.hasResponse()) {
            onError(publication,
                    exceptions::JoynrRuntimeException(
                            "Publication object had no response, discarded message"));
            return;
        }

        std::tuple<T, Ts...> responseTuple;
        try {
            callGetResponse(responseTuple, publication, std::index_sequence_for<T, Ts...>{});
        } catch (const std::exception& exception) {
            onError(publication, exceptions::JoynrRuntimeException(exception.what()));
            return;
        }

        callOnSucces(publication, std::move(responseTuple), std::index_sequence_for<T, Ts...>{});
    }

    template <std::size_t... Indices>
    void callOnSucces(const BasePublication& publication,
                      std::tuple<T, Ts...>&& response,
                      std::index_sequence<Indices...>)
    {
        onSuccess(publication, std::move(std::get<Indices>(response))...);
    }

    template <std::size_t... Indices>
    static void callGetResponse(std::tuple<T, Ts...>& response,
                                BasePublication& publication,
                                std::index_sequence<Indices...>)
    {
        publication.getResponse(std::get<Indices>(response)...);
    }
};

} // namespace joynr
#endif // SUBSCRIPTIONCALLBACK_H
