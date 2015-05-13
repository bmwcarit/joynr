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
#ifndef REPLYCALLER_H
#define REPLYCALLER_H

#include "joynr/IReplyCaller.h"

#include <typeinfo>
#include <QMetaType>
#include <QSharedPointer>
#include "joynr/RequestStatus.h"
#include <functional>

namespace joynr
{

template <class T>
/**
 * @brief This template class is the implementation for IReplyCaller for all types.
 * T is the desired type that the response should be converted to.
 *
 */
class ReplyCaller : public IReplyCaller
{
public:
    ReplyCaller(std::function<void(const joynr::RequestStatus& status, const T& returnValue)>
                        callbackFct)
            : callbackFct(callbackFct), hasTimeOutOccurred(false)
    {
    }

    ~ReplyCaller()
    {
    }

    void returnValue(const T& payload)
    {
        if (!hasTimeOutOccurred && callbackFct) {
            RequestStatus status(RequestStatusCode::OK);
            callbackFct(status, payload);
        }
    }

    void timeOut()
    {
        hasTimeOutOccurred = true;
        callbackFct(RequestStatus(RequestStatusCode::ERROR_TIME_OUT_WAITING_FOR_RESPONSE),
                    defaultValue);
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
    std::function<void(const joynr::RequestStatus& status, const T& returnValue)> callbackFct;
    bool hasTimeOutOccurred;
    static T defaultValue;
};

template <class T>
T ReplyCaller<T>::defaultValue;

template <>
/**
 * @brief Template specialisation for the void type.
 *
 */
class ReplyCaller<void> : public IReplyCaller
{
public:
    ReplyCaller(std::function<void(const joynr::RequestStatus& status)> callbackFct)
            : callbackFct(callbackFct), hasTimeOutOccurred(false)
    {
    }

    ~ReplyCaller()
    {
    }

    void returnValue()
    {
        if (!hasTimeOutOccurred && callbackFct) {
            callbackFct(RequestStatus(RequestStatusCode::OK));
        }
    }

    void timeOut()
    {
        hasTimeOutOccurred = true;
        callbackFct(RequestStatus(RequestStatusCode::ERROR_TIME_OUT_WAITING_FOR_RESPONSE));
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
    std::function<void(const joynr::RequestStatus& status)> callbackFct;
    bool hasTimeOutOccurred;
};

} // namespace joynr
#endif // REPLYCALLER_H
