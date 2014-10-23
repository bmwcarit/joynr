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
#ifndef JSONCALLBACK_H
#define JSONCALLBACK_H

#include "joynr/ICallback.h"
#include "joynr/Reply.h"
#include "joynr/IReplyCaller.h"

namespace joynr
{

template <class T>
/**
 * @brief This class is used to bridge the user-typed ICallback interface to
 * a standard type ICallback.  This was needed so an API could be defined
 * with a parameter of type ICallback without needing a template parameter.
 *
 * T is the type that the Reply will be converted to for the callback.
 */
class JsonCallback : public ICallback<Reply>
{

public:
    JsonCallback(QSharedPointer<ICallback<T>> callback) : callback(callback)
    {
    }

    ~JsonCallback()
    {
    }

    void onSuccess(const RequestStatus status, Reply result)
    {
        T typedResult = result.getResponse().value<T>();
        callback->onSuccess(status, typedResult);
    }

    void onFailure(const RequestStatus status)
    {
        callback->onFailure(status);
    }

private:
    QSharedPointer<ICallback<T>> callback;
};

template <>
/**
 * @brief The "void" specialisation of the JsonCallback<T> class.
 *
 */
class JsonCallback<void> : public ICallback<Reply>
{
public:
    JsonCallback(QSharedPointer<ICallback<void>> callback) : callback(callback)
    {
    }

    ~JsonCallback()
    {
    }

    void onSuccess(const RequestStatus status, Reply result)
    {
        Q_UNUSED(result);
        callback->onSuccess(status);
    }

    void onFailure(const RequestStatus status)
    {
        callback->onFailure(status);
    }

private:
    QSharedPointer<ICallback<void>> callback;
};

} // namespace joynr
#endif // JSONCALLBACK_H
