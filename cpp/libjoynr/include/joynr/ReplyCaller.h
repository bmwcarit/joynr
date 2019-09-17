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
#ifndef REPLYCALLER_H
#define REPLYCALLER_H

#include <functional>
#include <memory>

#include "joynr/IReplyCaller.h"
#include "joynr/ReplyInterpreter.h"
#include "joynr/exceptions/JoynrException.h"

namespace joynr
{

class BaseReplyCaller : public IReplyCaller
{
public:
    BaseReplyCaller(std::function<void(const std::shared_ptr<exceptions::JoynrException>& error)>&&
                            errorFct)
            : _errorFct(std::move(errorFct)), _hasTimeOutOccurred(false)
    {
    }

    ~BaseReplyCaller() override = default;

    void returnError(const std::shared_ptr<exceptions::JoynrException>& error) override
    {
        _errorFct(error);
    }

    void timeOut() override
    {
        _hasTimeOutOccurred = true;
        _errorFct(std::make_shared<exceptions::JoynrTimeOutException>(
                "timeout waiting for the response"));
    }

protected:
    std::function<void(const std::shared_ptr<exceptions::JoynrException>& error)> _errorFct;
    bool _hasTimeOutOccurred;
};

template <class... Ts>
/**
 * @brief This template class is the implementation for IReplyCaller for all types.
 * T is the desired type that the response should be converted to.
 *
 */
class ReplyCaller : public BaseReplyCaller
{
public:
    ReplyCaller(
            std::function<void(const Ts&...)> callbackFct,
            std::function<void(const std::shared_ptr<exceptions::JoynrException>& error)> errorFct)
            : BaseReplyCaller(std::move(errorFct)), _callbackFct(std::move(callbackFct))
    {
    }

    ~ReplyCaller() override = default;

    void returnValue(const Ts&... payload)
    {
        if (!_hasTimeOutOccurred && _callbackFct) {
            _callbackFct(payload...);
        }
    }

    void execute(Reply&& reply) override
    {
        ReplyInterpreter<Ts...>::execute(*this, std::move(reply));
    }

private:
    std::function<void(const Ts&... returnValue)> _callbackFct;
};

template <>
/**
 * @brief Template specialisation for the void type.
 *
 */
class ReplyCaller<void> : public BaseReplyCaller
{
public:
    ReplyCaller(
            std::function<void()> callbackFct,
            std::function<void(const std::shared_ptr<exceptions::JoynrException>& error)> errorFct)
            : BaseReplyCaller(std::move(errorFct)), _callbackFct(std::move(callbackFct))
    {
    }

    ~ReplyCaller() override = default;

    void returnValue()
    {
        if (!_hasTimeOutOccurred && _callbackFct) {
            _callbackFct();
        }
    }

    void execute(Reply&& reply) override
    {
        ReplyInterpreter<void>::execute(*this, std::move(reply));
    }

private:
    std::function<void()> _callbackFct;
};

} // namespace joynr
#endif // REPLYCALLER_H
