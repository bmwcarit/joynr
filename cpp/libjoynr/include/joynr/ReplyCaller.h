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
#include "joynr/Util.h"

namespace joynr
{

template <class... Ts>
/**
 * @brief This template class is the implementation for IReplyCaller for all types.
 * T is the desired type that the response should be converted to.
 *
 */
class ReplyCaller : public IReplyCaller
{
public:
    ReplyCaller(std::function<void(const joynr::RequestStatus& status, const Ts&...)> callbackFct,
                std::function<void(const joynr::RequestStatus& status)> errorFct)
            : callbackFct(callbackFct), errorFct(errorFct), hasTimeOutOccurred(false)
    {
    }

    ~ReplyCaller()
    {
    }

    void returnValue(const Ts&... payload)
    {
        if (!hasTimeOutOccurred && callbackFct) {
            RequestStatus status(RequestStatusCode::OK);
            callbackFct(status, payload...);
        }
    }

    void timeOut()
    {
        hasTimeOutOccurred = true;

        errorFct(RequestStatus(RequestStatusCode::ERROR_TIME_OUT_WAITING_FOR_RESPONSE));
    }

    int getTypeId() const
    {
        return Util::getTypeId<Ts...>();
    }

private:
    std::function<void(const joynr::RequestStatus& status, const Ts&... returnValue)> callbackFct;
    std::function<void(const joynr::RequestStatus& status)> errorFct;
    bool hasTimeOutOccurred;
};

template <>
/**
 * @brief Template specialisation for the void type.
 *
 */
class ReplyCaller<void> : public IReplyCaller
{
public:
    ReplyCaller(std::function<void(const joynr::RequestStatus& status)> callbackFct,
                std::function<void(const joynr::RequestStatus& status)> errorFct)
            : callbackFct(callbackFct), errorFct(errorFct), hasTimeOutOccurred(false)
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
        errorFct(RequestStatus(RequestStatusCode::ERROR_TIME_OUT_WAITING_FOR_RESPONSE));
    }

    int getTypeId() const
    {
        return Util::getTypeId<void>();
    }

private:
    std::function<void(const joynr::RequestStatus& status)> callbackFct;
    std::function<void(const joynr::RequestStatus& status)> errorFct;
    bool hasTimeOutOccurred;
};

} // namespace joynr
#endif // REPLYCALLER_H
