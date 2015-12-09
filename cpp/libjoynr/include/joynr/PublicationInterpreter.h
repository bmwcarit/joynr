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

#include "joynr/IPublicationInterpreter.h"
#include "joynr/joynrlogging.h"
#include "joynr/SubscriptionCallback.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/Util.h"
#include "joynr/exceptions/JoynrExceptionUtil.h"
#include <functional>

#include <cassert>
#include <memory>

namespace joynr
{

template <class... Ts>
class PublicationInterpreter : public IPublicationInterpreter
{
public:
    PublicationInterpreter()
    {
    }
    void execute(std::shared_ptr<ISubscriptionCallback> callback,
                 const SubscriptionPublication& subscriptionPublication)
    {
        assert(callback);

        const Variant& error = subscriptionPublication.getError();
        if (!error.isEmpty()) {
            callback->onError(
                    joynr::exceptions::JoynrExceptionUtil::extractJoynrRuntimeException(error));
            return;
        }

        std::vector<Variant> response = subscriptionPublication.getResponse();
        if (response.empty()) {
            LOG_ERROR(logger, "Publication object has no response, discarding message");
            exceptions::JoynrRuntimeException error(
                    "Publication object had no response, discarded message");
            callback->onError(error);
            return;
        }

        std::shared_ptr<SubscriptionCallback<Ts...>> typedCallbackQsp =
                std::dynamic_pointer_cast<SubscriptionCallback<Ts...>>(callback);

        std::tuple<Ts...> values = Util::toValueTuple<Ts...>(response);
        auto func = std::mem_fn(&SubscriptionCallback<Ts...>::onSuccess);

        Util::expandTupleIntoFunctionArguments(func, typedCallbackQsp, values);
    }

private:
    static joynr_logging::Logger* logger;
};

template <class... Ts>
joynr_logging::Logger* PublicationInterpreter<Ts...>::logger =
        joynr_logging::Logging::getInstance()->getLogger("MSG", "PublicationInterpreter");

/**
  * Class that handles conversion of enum publications
  * Template parameter T is the Enum wrapper class
  */
template <class T>
class EnumPublicationInterpreter : public IPublicationInterpreter
{
public:
    EnumPublicationInterpreter()
    {
    }
    void execute(std::shared_ptr<ISubscriptionCallback> callback,
                 const SubscriptionPublication& subscriptionPublication)
    {
        assert(callback);

        const Variant& error = subscriptionPublication.getError();
        if (!error.isEmpty()) {
            callback->onError(
                    joynr::exceptions::JoynrExceptionUtil::extractJoynrRuntimeException(error));
            return;
        }

        if (subscriptionPublication.getResponse().empty()) {
            LOG_ERROR(logger, "Publication object has no response, discarding message");
            exceptions::JoynrRuntimeException error(
                    "Publication object had no response, discarded message");
            callback->onError(error);
            return;
        }

        typename T::Enum value =
                Util::convertVariantToEnum<T>(subscriptionPublication.getResponse().front());

        std::shared_ptr<SubscriptionCallback<typename T::Enum>> typedCallbackQsp =
                std::dynamic_pointer_cast<SubscriptionCallback<typename T::Enum>>(callback);

        // value is copied in onSuccess
        // LOG_TRACE(logger, "Publication received: notifying attribute changed");
        typedCallbackQsp->onSuccess(value);
    }

private:
    static joynr_logging::Logger* logger;
};

template <class T>
joynr_logging::Logger* EnumPublicationInterpreter<T>::logger =
        joynr_logging::Logging::getInstance()->getLogger("MSG", "EnumPublicationInterpreter");

template <class T>
class EnumPublicationInterpreter<std::vector<T>> : public IPublicationInterpreter
{
public:
    EnumPublicationInterpreter()
    {
    }

    void execute(std::shared_ptr<ISubscriptionCallback> callback,
                 const SubscriptionPublication& subscriptionPublication)
    {
        assert(callback);

        const Variant& error = subscriptionPublication.getError();
        if (!error.isEmpty()) {
            callback->onError(
                    joynr::exceptions::JoynrExceptionUtil::extractJoynrRuntimeException(error));
            return;
        }

        std::vector<Variant> qvList = subscriptionPublication.getResponse();
        if (qvList.empty()) {
            LOG_ERROR(logger, "Publication object has no response, discarding message");
            exceptions::JoynrRuntimeException error(
                    "Publication object had no response, discarded message");
            callback->onError(error);
            return;
        }

        std::shared_ptr<SubscriptionCallback<std::vector<typename T::Enum>>> typedCallbackQsp =
                std::dynamic_pointer_cast<SubscriptionCallback<std::vector<typename T::Enum>>>(
                        callback);
        std::vector<typename T::Enum> valueList = Util::convertVariantVectorToEnumVector<T>(qvList);

        // value is copied in onSuccess
        typedCallbackQsp->onSuccess(valueList);
    }

private:
    static joynr_logging::Logger* logger;
};

template <class T>
joynr_logging::Logger* EnumPublicationInterpreter<std::vector<T>>::logger =
        joynr_logging::Logging::getInstance()->getLogger("MSG",
                                                         "EnumPublicationInterpreter<std::vector>");

} // namespace joynr
#endif // PUBLICATIONINTERPRETER_H
