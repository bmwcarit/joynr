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
#ifndef PUBLICATIONINTERPRETER_H
#define PUBLICATIONINTERPRETER_H

#include <cassert>
#include <memory>
#include <functional>

#include "joynr/IPublicationInterpreter.h"
#include "joynr/Logger.h"
#include "joynr/SubscriptionCallback.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/Util.h"
#include "joynr/exceptions/JoynrExceptionUtil.h"

namespace joynr
{

template <class... Ts>
class PublicationInterpreter : public IPublicationInterpreter
{
public:
    void execute(std::shared_ptr<ISubscriptionCallback> callback,
                 const SubscriptionPublication& subscriptionPublication) override
    {
        assert(callback);

        const Variant& error = subscriptionPublication.getErrorVariant();
        if (!error.isEmpty()) {
            callback->onError(
                    joynr::exceptions::JoynrExceptionUtil::extractJoynrRuntimeException(error));
            return;
        }

        std::vector<Variant> response = subscriptionPublication.getResponseVariant();
        if (response.empty()) {
            JOYNR_LOG_ERROR(logger, "Publication object has no response, discarding message");
            exceptions::JoynrRuntimeException error(
                    "Publication object had no response, discarded message");
            callback->onError(error);
            return;
        }
        std::shared_ptr<SubscriptionCallback<Ts...>> typedCallback =
                std::dynamic_pointer_cast<SubscriptionCallback<Ts...>>(callback);
        callOnSucces(response, typedCallback, std::index_sequence_for<Ts...>{});
    }

private:
    template <std::size_t... Indices>
    void callOnSucces(const std::vector<Variant>& response,
                      const std::shared_ptr<SubscriptionCallback<Ts...>>& typedCallback,
                      std::index_sequence<Indices...>)
    {
        typedCallback->onSuccess(util::valueOf<Ts>(response[Indices])...);
    }
    ADD_LOGGER(PublicationInterpreter);
};

template <>
class PublicationInterpreter<void> : public IPublicationInterpreter
{
public:
    void execute(std::shared_ptr<ISubscriptionCallback> callback,
                 const SubscriptionPublication& subscriptionPublication) override
    {
        assert(callback);

        const Variant& error = subscriptionPublication.getErrorVariant();
        if (!error.isEmpty()) {
            callback->onError(
                    joynr::exceptions::JoynrExceptionUtil::extractJoynrRuntimeException(error));
            return;
        }

        auto typedCallback = std::dynamic_pointer_cast<SubscriptionCallback<void>>(callback);
        typedCallback->onSuccess();
    }
    ADD_LOGGER(PublicationInterpreter);
};

template <class... Ts>
INIT_LOGGER(PublicationInterpreter<Ts...>);

/**
  * Class that handles conversion of enum publications
  * Template parameter T is the Enum wrapper class
  */
template <class T>
class EnumPublicationInterpreter : public IPublicationInterpreter
{
public:
    void execute(std::shared_ptr<ISubscriptionCallback> callback,
                 const SubscriptionPublication& subscriptionPublication) override
    {
        assert(callback);

        const Variant& error = subscriptionPublication.getErrorVariant();
        if (!error.isEmpty()) {
            callback->onError(
                    joynr::exceptions::JoynrExceptionUtil::extractJoynrRuntimeException(error));
            return;
        }

        if (subscriptionPublication.getResponseVariant().empty()) {
            JOYNR_LOG_ERROR(logger, "Publication object has no response, discarding message");
            exceptions::JoynrRuntimeException error(
                    "Publication object had no response, discarded message");
            callback->onError(error);
            return;
        }

        typename T::Enum value =
                util::convertVariantToEnum<T>(subscriptionPublication.getResponseVariant().front());

        std::shared_ptr<SubscriptionCallback<typename T::Enum>> typedCallback =
                std::dynamic_pointer_cast<SubscriptionCallback<typename T::Enum>>(callback);

        // value is copied in onSuccess
        // JOYNR_LOG_TRACE(logger, "Publication received: notifying attribute changed");
        typedCallback->onSuccess(value);
    }

private:
    ADD_LOGGER(EnumPublicationInterpreter);
};

template <class T>
INIT_LOGGER(EnumPublicationInterpreter<T>);

template <class T>
class EnumPublicationInterpreter<std::vector<T>> : public IPublicationInterpreter
{
public:
    void execute(std::shared_ptr<ISubscriptionCallback> callback,
                 const SubscriptionPublication& subscriptionPublication) override
    {
        assert(callback);

        const Variant& error = subscriptionPublication.getErrorVariant();
        if (!error.isEmpty()) {
            callback->onError(
                    joynr::exceptions::JoynrExceptionUtil::extractJoynrRuntimeException(error));
            return;
        }

        std::vector<Variant> qvList = subscriptionPublication.getResponseVariant();
        if (qvList.empty()) {
            JOYNR_LOG_ERROR(logger, "Publication object has no response, discarding message");
            exceptions::JoynrRuntimeException error(
                    "Publication object had no response, discarded message");
            callback->onError(error);
            return;
        }

        std::shared_ptr<SubscriptionCallback<std::vector<typename T::Enum>>> typedCallback =
                std::dynamic_pointer_cast<SubscriptionCallback<std::vector<typename T::Enum>>>(
                        callback);
        std::vector<typename T::Enum> valueList = util::convertVariantVectorToEnumVector<T>(qvList);

        // value is copied in onSuccess
        typedCallback->onSuccess(valueList);
    }

private:
    ADD_LOGGER(EnumPublicationInterpreter);
};

template <class T>
INIT_LOGGER(EnumPublicationInterpreter<std::vector<T>>);

} // namespace joynr
#endif // PUBLICATIONINTERPRETER_H
