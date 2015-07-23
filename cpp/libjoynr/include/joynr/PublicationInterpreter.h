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
#ifndef PUBLICATIONINTERPRETER_H
#define PUBLICATIONINTERPRETER_H

#include "joynr/IPublicationInterpreter.h"
#include "joynr/joynrlogging.h"
#include "joynr/SubscriptionCallback.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/Util.h"
#include <functional>

#include <cassert>

namespace joynr
{

using namespace joynr_logging;

template <class... Ts>
class PublicationInterpreter : public IPublicationInterpreter
{
public:
    PublicationInterpreter()
    {
    }
    void execute(QSharedPointer<ISubscriptionCallback> callback,
                 const SubscriptionPublication& subscriptionPublication)
    {
        assert(!callback.isNull());

        QList<QVariant> response = subscriptionPublication.getResponse();

        QSharedPointer<SubscriptionCallback<Ts...>> typedCallbackQsp =
                callback.dynamicCast<SubscriptionCallback<Ts...>>();

        std::tuple<Ts...> values = Util::toValueTuple<Ts...>(response);
        auto func = std::mem_fn(&SubscriptionCallback<Ts...>::onSuccess);

        Util::expandTupleIntoFunctionArguments(func, typedCallbackQsp, values);
    }

private:
    static joynr_logging::Logger* logger;
};

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
    void execute(QSharedPointer<ISubscriptionCallback> callback,
                 const SubscriptionPublication& subscriptionPublication)
    {
        assert(!callback.isNull());

        typename T::Enum value =
                Util::convertVariantToEnum<T>(subscriptionPublication.getResponse().first());

        QSharedPointer<SubscriptionCallback<typename T::Enum>> typedCallbackQsp =
                callback.dynamicCast<SubscriptionCallback<typename T::Enum>>();

        // value is copied in onSuccess
        // LOG_TRACE(logger, "Publication received: notifying attribute changed");
        typedCallbackQsp->onSuccess(value);
    }

private:
    static joynr_logging::Logger* logger;
};

template <class T>
class EnumPublicationInterpreter<QList<T>> : public IPublicationInterpreter
{
public:
    EnumPublicationInterpreter()
    {
    }

    void execute(QSharedPointer<ISubscriptionCallback> callback,
                 const SubscriptionPublication& subscriptionPublication)
    {
        assert(!callback.isNull());

        QList<QVariant> qvList = subscriptionPublication.getResponse();
        QSharedPointer<SubscriptionCallback<QList<typename T::Enum>>> typedCallbackQsp =
                callback.dynamicCast<SubscriptionCallback<QList<typename T::Enum>>>();
        QList<typename T::Enum> valueList = Util::convertVariantListToEnumList<T>(qvList);

        // value is copied in onSuccess
        typedCallbackQsp->onSuccess(valueList);
    }

private:
};

} // namespace joynr
#endif // PUBLICATIONINTERPRETER_H
