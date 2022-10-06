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
#ifndef IREQUESTINTERPRETER_H
#define IREQUESTINTERPRETER_H

#include <functional>
#include <memory>

namespace joynr
{

class RequestCaller;
class Request;
class OneWayRequest;
class BaseReply;

namespace exceptions
{
class JoynrException;
} // namespace exceptions

/**
 * Common interface for all @class <Intf>RequestInterpreter.
 */
class IRequestInterpreter
{
public:
    virtual ~IRequestInterpreter() = default;

    /**
     * Executes request on the @param requestCaller object.
     */
    virtual void execute(
            std::shared_ptr<RequestCaller> requestCaller,
            Request& request,
            std::function<void(BaseReply&& outParams)>&& onSuccess,
            std::function<void(const std::shared_ptr<exceptions::JoynrException>& exception)>&&
                    onError) = 0;

    /**
     * Executes fire-and-forget request on the @param requestCaller object.
     */
    virtual void execute(std::shared_ptr<RequestCaller> requestCaller, OneWayRequest& request) = 0;
};

} // namespace joynr
#endif // IREQUESTINTERPRETER_H
