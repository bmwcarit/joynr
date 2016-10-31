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
#ifndef SUBSCRIPTIONCALLBACK_H
#define SUBSCRIPTIONCALLBACK_H

#include <memory>
#include <tuple>

#include "joynr/ISubscriptionCallback.h"
#include "joynr/ISubscriptionListener.h"
#include "ISubscriptionManager.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/BasePublication.h"
#include "joynr/Future.h"
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
                                  ISubscriptionManager* subscriptionManager)
            : subscriptionId(subscriptionId),
              future(std::move(future)),
              subscriptionManager(subscriptionManager)
    {
    }

    void onError(const exceptions::JoynrRuntimeException& error) override
    {
        static_cast<Derived*>(this)->onError(error);
    }

    template <typename Holder = T>
    std::enable_if_t<std::is_void<Holder>::value, void> onSuccess()
    {
        static_cast<Derived*>(this)->onSuccess();
    }

    template <typename Holder = T>
    std::enable_if_t<!std::is_void<Holder>::value, void> onSuccess(const Holder& value,
                                                                   const Ts&... values)
    {
        static_cast<Derived*>(this)->onSuccess(value, values...);
    }

    void execute(BasePublication&& publication) override
    {
        interpret(std::move(publication));
    }

    void execute(const SubscriptionReply& subscriptionReply) override
    {
        std::shared_ptr<exceptions::JoynrRuntimeException> error = subscriptionReply.getError();
        std::shared_ptr<ISubscriptionListenerBase> listener =
                subscriptionManager->getSubscriptionListener(subscriptionId);
        if (error) {
            subscriptionManager->unregisterSubscription(subscriptionReply.getSubscriptionId());
            future->onError(error);
            listener->onError(*error);
        } else {
            future->onSuccess(subscriptionReply.getSubscriptionId());
            listener->onSubscribed(subscriptionReply.getSubscriptionId());
        }
    }

protected:
    std::string subscriptionId;
    std::shared_ptr<Future<std::string>> future;
    ISubscriptionManager* subscriptionManager;

private:
    DISALLOW_COPY_AND_ASSIGN(SubscriptionCallback);
    template <typename Holder = T>
    std::enable_if_t<std::is_void<Holder>::value, void> interpret(BasePublication&& publication)
    {
        std::shared_ptr<exceptions::JoynrRuntimeException> error = publication.getError();
        if (error) {
            onError(*error);
            return;
        }
        onSuccess();
    }

    template <typename Holder = T>
    std::enable_if_t<!std::is_void<Holder>::value, void> interpret(BasePublication&& publication)
    {
        std::shared_ptr<exceptions::JoynrRuntimeException> error = publication.getError();
        if (error) {
            onError(*error);
            return;
        }

        if (!publication.hasResponse()) {
            onError(exceptions::JoynrRuntimeException(
                    "Publication object had no response, discarded message"));
            return;
        }

        std::tuple<T, Ts...> responseTuple;
        try {
            callGetResponse(
                    responseTuple, std::move(publication), std::index_sequence_for<T, Ts...>{});
        } catch (const std::exception& exception) {
            onError(exceptions::JoynrRuntimeException(exception.what()));
            return;
        }

        callOnSucces(std::move(responseTuple), std::index_sequence_for<T, Ts...>{});
    }

    template <std::size_t... Indices>
    void callOnSucces(std::tuple<T, Ts...>&& response, std::index_sequence<Indices...>)
    {
        onSuccess(std::move(std::get<Indices>(response))...);
    }

    template <std::size_t... Indices>
    static void callGetResponse(std::tuple<T, Ts...>& response,
                                BasePublication&& publication,
                                std::index_sequence<Indices...>)
    {
        publication.getResponse(std::get<Indices>(response)...);
    }
};

} // namespace joynr
#endif // SUBSCRIPTIONCALLBACK_H
