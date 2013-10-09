/*
 * #%L
 * joynr::C++
 * $Id:$
 * $HeadURL:$
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
#ifndef REPLYCALLER_H
#define REPLYCALLER_H

#include "joynr/IReplyCaller.h"
#include "joynr/ICallback.h"

#include <typeinfo>
#include <QMetaType>
#include <QSharedPointer>

namespace joynr {


template <class T>
/**
 * @brief This template class is the implementation for IReplyCaller for all types.
 * T is the desired type that the response should be converted to.
 *
 */
class ReplyCaller : public IReplyCaller{
public:

    ReplyCaller(QSharedPointer<ICallback<T> > callback)
        : callback(callback),
          hasTimeOutOccurred(false){
    }

    ~ReplyCaller() { }


    void returnValue(const T& payload)
    {
        if (!hasTimeOutOccurred) {
            callback->onSuccess(RequestStatus(RequestStatusCode::OK), payload);
        }
    }

    void timeOut()
    {
        hasTimeOutOccurred = true;
        callback->onFailure(RequestStatus(RequestStatusCode::ERROR_TIME_OUT_WAITING_FOR_RESPONSE));
    }

    QString getTypeName() const
    {
        QString name = QMetaType::typeName(getTypeId());
        return name;
    }

    int getTypeId() const
    {
        return qMetaTypeId<T>();
    }

private:
    QSharedPointer<ICallback<T> > callback;
    bool hasTimeOutOccurred;
};

template <>
/**
 * @brief Template specialisation for the void type.
 *
 */
class ReplyCaller <void> : public IReplyCaller {
public:

    ReplyCaller(QSharedPointer<ICallback<void> > callback)
        : callback(callback),
          hasTimeOutOccurred(false)
    {
    }

    ~ReplyCaller()
    {
    }


    void returnValue()
    {
        if (!hasTimeOutOccurred)
        {
            callback->onSuccess(RequestStatus(RequestStatusCode::OK));
        }
    }

    void timeOut()
    {
        hasTimeOutOccurred = true;
        callback->onFailure(RequestStatus(RequestStatusCode::ERROR_TIME_OUT_WAITING_FOR_RESPONSE));
    }

    QString getTypeName() const
    {
        QString name = QMetaType::typeName(getTypeId());
        return name;
    }

    int getTypeId() const
    {
        return qMetaTypeId<void>();
    }

private:
    QSharedPointer<ICallback<void> > callback;
    bool hasTimeOutOccurred;
};


} // namespace joynr
#endif //REPLYCALLER_H
