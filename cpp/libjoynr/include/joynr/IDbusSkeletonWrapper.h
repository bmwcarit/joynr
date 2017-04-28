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
#ifndef IDBUSSKELETONWRAPPER_H
#define IDBUSSKELETONWRAPPER_H

#include <chrono>
#include <string>
#include <CommonAPI/CommonAPI.h>

#include "joynr/JoynrExport.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Logger.h"

namespace joynr
{

template <class _SkeletonClass, class _CallBackClass>
class JOYNR_EXPORT IDbusSkeletonWrapper
{
public:
    IDbusSkeletonWrapper(_CallBackClass& callBack, std::string serviceAddress)
            : // factory(NULL),
              serviceAddress(serviceAddress)
    {
        JOYNR_LOG_TRACE(logger, "Registering dbus skeleton on address: {}", serviceAddress);

        // create the skeleton
        std::shared_ptr<_SkeletonClass> skeleton = std::make_shared<_SkeletonClass>(callBack);

        // register skeleton
        auto runtime = CommonAPI::Runtime::load("DBus");
        bool success = runtime->getServicePublisher()->registerService(
                skeleton, serviceAddress, runtime->createFactory());
        // wait some time so that the service is registered and ready to use on dbus level
        std::this_thread::sleep_for(std::chrono::milliseconds(25));

        if (success) {
            JOYNR_LOG_INFO(logger, "registering service {}: SUCCESS", serviceAddress);
        } else {
            JOYNR_LOG_FATAL(logger, "registering service {} : ERROR", serviceAddress);
        }
    }

    ~IDbusSkeletonWrapper()
    {
        JOYNR_LOG_TRACE(logger, "Unregistering dbus skeleton from address: {}", serviceAddress);

        auto runtime = CommonAPI::Runtime::load("DBus");
        bool success = runtime->getServicePublisher()->unregisterService(serviceAddress);
        // wait some time so that the service is unregistered on dbus level
        std::this_thread::sleep_for(std::chrono::milliseconds(25));

        if (success) {
            JOYNR_LOG_INFO(logger, "unregistering service {}: SUCCESS", serviceAddress);
        } else {
            JOYNR_LOG_FATAL(logger, "unregistering service {}: ERROR", serviceAddress);
        }
    }

    void logMethodCall(const std::string& method, const std::string& adapter)
    {
        JOYNR_LOG_INFO(logger, "Call method {}:{}-> {}", adapter, serviceAddress, method);
    }

private:
    DISALLOW_COPY_AND_ASSIGN(IDbusSkeletonWrapper);
    std::string serviceAddress;
    ADD_LOGGER(IDbusSkeletonWrapper);
};

template <class _SkeletonClass, class _CallBackClass>
INIT_LOGGER(SINGLE_MACRO_ARG(IDbusSkeletonWrapper<_SkeletonClass, _CallBackClass>));

} // namespace joynr
#endif // IDBUSSKELETONWRAPPER_H
